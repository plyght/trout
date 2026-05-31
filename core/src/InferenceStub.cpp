#include "trout/Inference.h"
#include "trout/Log.h"

#include <memory>

namespace trout {

// Deterministic development backend used when llama.cpp is not compiled in or
// no model is available. It echoes a short canned continuation so the rest of
// the pipeline (overlay, acceptance, stats) can be exercised end to end.
class StubBackend : public InferenceBackend {
public:
    bool load(const std::string& modelPath, const InferenceParams&, std::string*) override {
        loaded_ = true;
        TROUT_INFO("StubBackend loaded (no real inference): " << modelPath);
        return true;
    }
    bool isLoaded() const override { return loaded_; }
    void unload() override { loaded_ = false; }

    std::string complete(const std::string& prompt, const InferenceParams& params,
                         const TokenCallback& onToken, std::atomic<bool>& cancel) override {
        // Produce a few plausible filler words so word-by-word accept is testable.
        static const char* words[] = {" and", " then", " we", " can", " continue", " here."};
        std::string out;
        for (const char* w : words) {
            if (cancel.load()) break;
            out += w;
            if (onToken && !onToken(w)) break;
            if ((int)out.size() >= params.maxTokens * 4) break;
        }
        return out;
    }

private:
    bool loaded_ = false;
};

#ifndef TROUT_WITH_LLAMA
std::unique_ptr<InferenceBackend> makeInferenceBackend() {
    return std::make_unique<StubBackend>();
}
#endif

std::unique_ptr<InferenceBackend> makeStubBackend() {
    return std::make_unique<StubBackend>();
}

} // namespace trout
