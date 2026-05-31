#pragma once

#include "trout/Settings.h"
#include "trout/Inference.h"

#include <string>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>

namespace trout {

class Store;

// The text context captured from a focused field by the macOS layer.
struct CompletionContext {
    std::string bundleId;        // focused app bundle id
    std::string domain;          // browser domain if known
    std::string textBeforeCursor;
    std::string textAfterCursor; // for mid-line (phase 3); empty for end-of-line
    bool isSecureField = false;  // password/secure -> never complete
    std::string clipboardText;   // only populated if user enabled clipboard ctx
    std::string screenContext;   // only populated if user enabled screenshot ctx
};

// A produced suggestion plus word boundaries for word-by-word acceptance.
struct Suggestion {
    std::string fullText;
    std::vector<size_t> wordEnds; // byte offsets marking end of each word chunk
    uint64_t requestId = 0;
    bool typoSuppressed = false;  // engine declined due to suspected typo
};

// Delivered asynchronously to the UI/overlay layer.
using SuggestionCallback = std::function<void(const Suggestion&)>;

// Orchestrates context -> prompt -> inference -> suggestion. Requests are
// debounced and run on a single worker thread; a newer request cancels an
// in-flight older one to keep latency low.
class Engine {
public:
    Engine(Settings* settings, Store* store);
    ~Engine();

    bool start(std::string* err);          // loads selected model
    void stop();
    bool reloadModel(std::string* err);    // re-reads selected model from settings

    // Request a completion. Returns the request id. Cancels prior request.
    uint64_t requestCompletion(const CompletionContext& ctx);
    void cancelAll();

    void setSuggestionCallback(SuggestionCallback cb);

    bool modelReady() const { return backend_ && backend_->isLoaded(); }
    std::string loadedModelPath() const;

    // Record that the user accepted some text (for stats/personalization).
    void recordAcceptance(int words, int characters, const std::string& bundleId);

private:
    void workerLoop();
    std::string buildPrompt(const CompletionContext& ctx) const;
    Suggestion postProcess(const std::string& raw, uint64_t reqId) const;
    bool suspectedTypo(const std::string& textBeforeCursor) const;
    InferenceParams paramsForLength(CompletionLength len) const;

    Settings* settings_;
    Store* store_;
    std::unique_ptr<InferenceBackend> backend_;
    std::string loadedPath_;

    std::thread worker_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool stop_ = false;
    bool hasPending_ = false;
    CompletionContext pending_;
    std::atomic<uint64_t> nextRequestId_{1};
    std::atomic<uint64_t> activeRequestId_{0};
    std::atomic<bool> cancelFlag_{false};

    SuggestionCallback callback_;
    std::mutex cbMutex_;
};

} // namespace trout
