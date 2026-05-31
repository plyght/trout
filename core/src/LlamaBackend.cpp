#include "trout/Inference.h"
#include "trout/Log.h"

#include "llama.h"

#include <memory>
#include <vector>
#include <mutex>

namespace trout {

std::unique_ptr<InferenceBackend> makeStubBackend();

namespace {
std::once_flag g_backendInit;
void ensureBackend() {
    std::call_once(g_backendInit, []{ llama_backend_init(); });
}
} // namespace

class LlamaBackend : public InferenceBackend {
public:
    ~LlamaBackend() override { unload(); }

    bool load(const std::string& modelPath, const InferenceParams& params, std::string* err) override {
        ensureBackend();
        unload();

        llama_model_params mp = llama_model_default_params();
        mp.n_gpu_layers = params.gpuLayers;
        model_ = llama_model_load_from_file(modelPath.c_str(), mp);
        if (!model_) { if (err) *err = "failed to load model: " + modelPath; return false; }

        llama_context_params cp = llama_context_default_params();
        cp.n_ctx = params.contextTokens;
        cp.n_batch = params.contextTokens;
        if (params.threads > 0) { cp.n_threads = params.threads; cp.n_threads_batch = params.threads; }
        ctx_ = llama_init_from_model(model_, cp);
        if (!ctx_) { if (err) *err = "failed to create context"; unload(); return false; }

        vocab_ = llama_model_get_vocab(model_);
        path_ = modelPath;
        TROUT_INFO("llama model loaded: " << modelPath << " ctx=" << params.contextTokens);
        return true;
    }

    bool isLoaded() const override { return ctx_ != nullptr; }

    void unload() override {
        if (ctx_) { llama_free(ctx_); ctx_ = nullptr; }
        if (model_) { llama_model_free(model_); model_ = nullptr; }
        vocab_ = nullptr;
        path_.clear();
    }

    std::string complete(const std::string& prompt, const InferenceParams& params,
                         const TokenCallback& onToken, std::atomic<bool>& cancel) override {
        std::lock_guard<std::mutex> lk(genMutex_);
        if (!ctx_) return "";

        // Reset KV cache for this fresh request.
        llama_memory_seq_rm(llama_get_memory(ctx_), -1, -1, -1);

        std::vector<llama_token> tokens = tokenize(prompt, true);
        if (tokens.empty()) return "";

        const int nCtx = (int)llama_n_ctx(ctx_);
        if ((int)tokens.size() >= nCtx - params.maxTokens) {
            // Keep only the most recent tokens that fit.
            int keep = nCtx - params.maxTokens - 1;
            if (keep < 1) return "";
            tokens.erase(tokens.begin(), tokens.end() - keep);
        }

        llama_sampler* smpl = makeSampler(params);

        // Decode the prompt.
        llama_batch batch = llama_batch_get_one(tokens.data(), (int)tokens.size());
        if (llama_decode(ctx_, batch) != 0) { llama_sampler_free(smpl); return ""; }

        std::string out;
        for (int i = 0; i < params.maxTokens; ++i) {
            if (cancel.load()) break;
            llama_token tok = llama_sampler_sample(smpl, ctx_, -1);
            if (llama_vocab_is_eog(vocab_, tok)) break;

            std::string piece = tokenToPiece(tok);
            out += piece;
            if (onToken && !onToken(piece)) break;

            // Stop at a sentence boundary for short, in-flow completions.
            if (!params.stopSentence.empty() &&
                out.find(params.stopSentence) != std::string::npos) break;

            llama_token next[1] = { tok };
            llama_batch nb = llama_batch_get_one(next, 1);
            if (llama_decode(ctx_, nb) != 0) break;
        }

        llama_sampler_free(smpl);
        return out;
    }

private:
    std::vector<llama_token> tokenize(const std::string& text, bool addSpecial) {
        int n = -llama_tokenize(vocab_, text.c_str(), (int)text.size(), nullptr, 0, addSpecial, true);
        std::vector<llama_token> toks(n);
        int got = llama_tokenize(vocab_, text.c_str(), (int)text.size(),
                                 toks.data(), (int)toks.size(), addSpecial, true);
        if (got < 0) return {};
        toks.resize(got);
        return toks;
    }

    std::string tokenToPiece(llama_token tok) {
        char buf[256];
        int n = llama_token_to_piece(vocab_, tok, buf, sizeof(buf), 0, false);
        if (n < 0) return "";
        return std::string(buf, n);
    }

    llama_sampler* makeSampler(const InferenceParams& p) {
        auto sp = llama_sampler_chain_default_params();
        llama_sampler* chain = llama_sampler_chain_init(sp);
        if (p.temperature <= 0.0f) {
            llama_sampler_chain_add(chain, llama_sampler_init_greedy());
        } else {
            llama_sampler_chain_add(chain, llama_sampler_init_top_k(p.topK));
            llama_sampler_chain_add(chain, llama_sampler_init_top_p(p.topP, 1));
            llama_sampler_chain_add(chain, llama_sampler_init_temp(p.temperature));
            llama_sampler_chain_add(chain, llama_sampler_init_dist(LLAMA_DEFAULT_SEED));
        }
        return chain;
    }

    llama_model* model_ = nullptr;
    llama_context* ctx_ = nullptr;
    const llama_vocab* vocab_ = nullptr;
    std::string path_;
    std::mutex genMutex_;
};

std::unique_ptr<InferenceBackend> makeInferenceBackend() {
    return std::make_unique<LlamaBackend>();
}

} // namespace trout
