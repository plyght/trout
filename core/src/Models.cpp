#include "trout/Models.h"
#include "trout/Log.h"

#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

namespace trout {

Models::Models(std::string modelsDir) : dir_(std::move(modelsDir)) {}

static std::string parseQuant(const std::string& name) {
    // Heuristic: look for a token like Q4_K_M / Q5_K_S / Q8_0 / F16.
    static const char* tags[] = {"Q2_K","Q3_K_S","Q3_K_M","Q3_K_L","Q4_0","Q4_K_S",
        "Q4_K_M","Q5_0","Q5_K_S","Q5_K_M","Q6_K","Q8_0","F16","F32","BF16"};
    std::string upper = name;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    for (const char* t : tags) if (upper.find(t) != std::string::npos) return t;
    return "";
}

std::vector<ModelInfo> Models::installed() const {
    std::vector<ModelInfo> out;
    std::error_code ec;
    if (!fs::exists(dir_, ec)) return out;
    for (auto& e : fs::directory_iterator(dir_, ec)) {
        if (!e.is_regular_file()) continue;
        auto p = e.path();
        if (p.extension() != ".gguf") continue;
        ModelInfo m;
        m.id = p.stem().string();
        m.displayName = m.id;
        m.path = p.string();
        m.sizeBytes = (uint64_t)e.file_size(ec);
        m.quant = parseQuant(m.id);
        m.present = true;
        out.push_back(m);
    }
    std::sort(out.begin(), out.end(), [](const ModelInfo& a, const ModelInfo& b){
        return a.displayName < b.displayName;
    });
    return out;
}

std::optional<ModelInfo> Models::find(const std::string& id) const {
    for (auto& m : installed()) if (m.id == id) return m;
    return std::nullopt;
}

std::vector<ModelCatalogEntry> Models::catalog() {
    return {
        {"qwen2.5-0.5b-instruct-q4_k_m",
         "Qwen2.5 0.5B Instruct (Q4_K_M)",
         "https://huggingface.co/Qwen/Qwen2.5-0.5B-Instruct-GGUF/resolve/main/qwen2.5-0.5b-instruct-q4_k_m.gguf",
         400ull * 1024 * 1024,
         "Fastest. Lowest memory. Best for weaker machines or degraded mode."},
        {"qwen2.5-1.5b-instruct-q4_k_m",
         "Qwen2.5 1.5B Instruct (Q4_K_M)",
         "https://huggingface.co/Qwen/Qwen2.5-1.5B-Instruct-GGUF/resolve/main/qwen2.5-1.5b-instruct-q4_k_m.gguf",
         1100ull * 1024 * 1024,
         "Good balance of latency and quality on M1/M2 with 16GB."},
        {"qwen2.5-3b-instruct-q4_k_m",
         "Qwen2.5 3B Instruct (Q4_K_M)",
         "https://huggingface.co/Qwen/Qwen2.5-3B-Instruct-GGUF/resolve/main/qwen2.5-3b-instruct-q4_k_m.gguf",
         2000ull * 1024 * 1024,
         "Higher quality completions; needs more memory and is more battery-intensive."},
    };
}

std::string Models::recommendForRam(uint64_t ramBytes) {
    const uint64_t GB = 1024ull * 1024 * 1024;
    if (ramBytes >= 32 * GB) return "qwen2.5-3b-instruct-q4_k_m";
    if (ramBytes >= 16 * GB) return "qwen2.5-1.5b-instruct-q4_k_m";
    return "qwen2.5-0.5b-instruct-q4_k_m";
}

std::optional<ModelInfo> Models::import(const std::string& srcPath, std::string* err) {
    std::error_code ec;
    fs::path src(srcPath);
    if (!fs::exists(src, ec)) { if (err) *err = "source file does not exist"; return std::nullopt; }
    if (src.extension() != ".gguf") { if (err) *err = "not a .gguf file"; return std::nullopt; }
    fs::create_directories(dir_, ec);
    fs::path dst = fs::path(dir_) / src.filename();
    fs::copy_file(src, dst, fs::copy_options::overwrite_existing, ec);
    if (ec) { if (err) *err = ec.message(); return std::nullopt; }
    return find(dst.stem().string());
}

} // namespace trout
