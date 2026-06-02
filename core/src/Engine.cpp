#include "trout/Engine.h"
#include "trout/Store.h"
#include "trout/Log.h"
#include "trout/PromptRequestFactory.h"
#include "trout/SuggestionTextNormalizer.h"
#include "trout/TypoSuppression.h"

#include <cctype>

namespace trout {

Engine::Engine(Settings* settings, Store* store)
    : settings_(settings), store_(store) {}

Engine::~Engine() { stop(); }

bool Engine::start(std::string* err) {
    backend_ = makeInferenceBackend();
    if (!reloadModel(err)) {
        // Not fatal: engine can run with no model and simply produce nothing.
        TROUT_WARN("engine started without a loaded model");
    }
    {
        std::lock_guard<std::mutex> lk(mutex_);
        stop_ = false;
    }
    worker_ = std::thread([this]{ workerLoop(); });
    return true;
}

void Engine::stop() {
    {
        std::lock_guard<std::mutex> lk(mutex_);
        if (stop_) return;
        stop_ = true;
        cancelFlag_.store(true);
    }
    cv_.notify_all();
    if (worker_.joinable()) worker_.join();
    if (backend_) backend_->unload();
}

bool Engine::reloadModel(std::string* err) {
    if (!backend_) backend_ = makeInferenceBackend();
    auto g = settings_->global();
    if (g.selectedModelId.empty()) { if (err) *err = "no model selected"; return false; }

    // The selected model id maps to <modelsDir>/<id>.gguf; the macOS layer sets
    // the models dir via Paths. We resolve through Settings indirectly: the UI
    // stores an absolute path hint in the model id when imported. Here we accept
    // either an absolute path or a bare id resolved against the models dir.
    std::string path = g.selectedModelId;
    if (path.find('/') == std::string::npos) {
        // Caller is expected to pass absolute path via selectedModelId for now.
        if (err) *err = "model id is not an absolute path";
        return false;
    }

    InferenceParams ip = paramsForLength(g.maxCompletionLength);
    if (!backend_->load(path, ip, err)) return false;
    loadedPath_ = path;
    return true;
}

std::string Engine::loadedModelPath() const { return loadedPath_; }

InferenceParams Engine::paramsForLength(CompletionLength len) const {
    InferenceParams p;
    switch (len) {
        case CompletionLength::Short:  p.maxTokens = 16; p.stopSentence = ". "; break;
        case CompletionLength::Medium: p.maxTokens = 40; break;
        case CompletionLength::Long:   p.maxTokens = 96; break;
    }
    return p;
}

void Engine::setSuggestionCallback(SuggestionCallback cb) {
    std::lock_guard<std::mutex> lk(cbMutex_);
    callback_ = std::move(cb);
}

uint64_t Engine::requestCompletion(const CompletionContext& ctx) {
    uint64_t id = nextRequestId_.fetch_add(1);
    {
        std::lock_guard<std::mutex> lk(mutex_);
        pending_ = ctx;
        hasPending_ = true;
        activeRequestId_.store(id);
        cancelFlag_.store(true); // cancel any in-flight generation
    }
    cv_.notify_all();
    return id;
}

void Engine::cancelAll() {
    cancelFlag_.store(true);
    std::lock_guard<std::mutex> lk(mutex_);
    hasPending_ = false;
}

void Engine::workerLoop() {
    while (true) {
        CompletionContext ctx;
        uint64_t reqId = 0;
        {
            std::unique_lock<std::mutex> lk(mutex_);
            cv_.wait(lk, [this]{ return stop_ || hasPending_; });
            if (stop_) return;
            ctx = pending_;
            hasPending_ = false;
            reqId = activeRequestId_.load();
            cancelFlag_.store(false);
        }

        if (ctx.isSecureField) continue;
        if (!settings_->completionsEnabledFor(ctx.bundleId)) continue;
        if (!backend_ || !backend_->isLoaded()) continue;

        auto g = settings_->global();
        if (g.hideOnSuspectedTypo && trout::suspectedTypo(ctx.textBeforeCursor)) {
            Suggestion s; s.requestId = reqId; s.typoSuppressed = true;
            std::lock_guard<std::mutex> lk(cbMutex_);
            if (callback_) callback_(s);
            continue;
        }

        std::string prompt = makePromptRequest(ctx, g, settings_->appProfile(ctx.bundleId)).prompt;
        InferenceParams ip = paramsForLength(g.maxCompletionLength);

        std::string raw = backend_->complete(prompt, ip, nullptr, cancelFlag_);
        if (cancelFlag_.load()) continue;          // superseded by newer request
        if (activeRequestId_.load() != reqId) continue;

        SuggestionNormalizationRequest normalizerRequest;
        normalizerRequest.raw = raw;
        normalizerRequest.prompt = prompt;
        normalizerRequest.precedingText = ctx.textBeforeCursor;
        normalizerRequest.trailingText = ctx.textAfterCursor;
        Suggestion s = postProcess(normalizeSuggestionText(normalizerRequest), reqId);
        if (s.fullText.empty()) continue;

        std::lock_guard<std::mutex> lk(cbMutex_);
        if (callback_) callback_(s);
    }
}

Suggestion Engine::postProcess(const std::string& raw, uint64_t reqId) const {
    // Trim leading whitespace/newlines and cut at the first newline so we keep a
    // single-line, in-flow continuation.
    std::string text = raw;
    size_t nl = text.find('\n');
    if (nl != std::string::npos) text = text.substr(0, nl);
    // Strip a leading space mismatch only if the prior char already had a space;
    // the macOS layer handles exact spacing on insertion.

    Suggestion s;
    s.requestId = reqId;
    s.fullText = text;

    // Compute word-end byte offsets for word-by-word acceptance: each boundary
    // is the index just after a run of non-space followed by optional space.
    size_t i = 0;
    const size_t n = text.size();
    while (i < n) {
        while (i < n && std::isspace((unsigned char)text[i])) ++i;       // leading space
        while (i < n && !std::isspace((unsigned char)text[i])) ++i;      // the word
        if (i < n && std::isspace((unsigned char)text[i])) ++i;          // trailing space
        s.wordEnds.push_back(i);
    }
    if (s.wordEnds.empty() && !text.empty()) s.wordEnds.push_back(text.size());
    return s;
}

void Engine::recordAcceptance(int words, int characters, const std::string& bundleId) {
    if (store_) {
        store_->recordAcceptance(words, characters);
        auto g = settings_->global();
        if (g.collectInputs) store_->recordTypingSample(bundleId, characters, true);
    }
}

} // namespace trout
