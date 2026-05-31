#pragma once

#include <string>
#include <functional>
#include <memory>
#include <atomic>

namespace trout {

struct InferenceParams {
    int maxTokens = 24;        // completion budget (short by default)
    float temperature = 0.4f;
    float topP = 0.9f;
    int topK = 40;
    int contextTokens = 2048;
    int threads = 0;           // 0 = auto
    int gpuLayers = 999;       // offload all to Metal by default on Apple Silicon
    std::string stopSentence;  // optional extra stop string
};

// Streaming token callback. Return false to request cancellation.
using TokenCallback = std::function<bool(const std::string& piece)>;

// Abstract local inference backend. The default implementation wraps
// llama.cpp; a stub backend is used when no model is loaded.
class InferenceBackend {
public:
    virtual ~InferenceBackend() = default;

    virtual bool load(const std::string& modelPath, const InferenceParams& params, std::string* err) = 0;
    virtual bool isLoaded() const = 0;
    virtual void unload() = 0;

    // Generate a completion for the given prompt. Calls onToken for each piece.
    // Honors cancel flag between tokens. Returns the full generated string.
    virtual std::string complete(const std::string& prompt,
                                 const InferenceParams& params,
                                 const TokenCallback& onToken,
                                 std::atomic<bool>& cancel) = 0;
};

// Factory: returns a llama.cpp backend if compiled with TROUT_WITH_LLAMA,
// otherwise a deterministic stub used for development and tests.
std::unique_ptr<InferenceBackend> makeInferenceBackend();

} // namespace trout
