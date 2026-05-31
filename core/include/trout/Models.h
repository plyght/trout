#pragma once

#include <string>
#include <vector>
#include <optional>
#include <functional>

namespace trout {

struct ModelInfo {
    std::string id;          // stable identifier (filename stem)
    std::string displayName;
    std::string path;        // absolute path to GGUF file
    uint64_t sizeBytes = 0;
    std::string quant;       // parsed quant tag if available (e.g. Q4_K_M)
    bool present = false;    // file exists on disk
};

// A curated registry entry the user can download. Kept minimal and local-first;
// downloads are explicit and use direct URLs (no telemetry).
struct ModelCatalogEntry {
    std::string id;
    std::string displayName;
    std::string url;
    uint64_t approxSizeBytes = 0;
    std::string notes;       // latency/memory guidance
};

class Models {
public:
    explicit Models(std::string modelsDir);

    // Scan the models directory for *.gguf files.
    std::vector<ModelInfo> installed() const;
    std::optional<ModelInfo> find(const std::string& id) const;

    // Built-in suggested catalog for first-run download flow.
    static std::vector<ModelCatalogEntry> catalog();

    // Returns the recommended catalog id given available RAM (bytes).
    static std::string recommendForRam(uint64_t ramBytes);

    // Import an existing GGUF by copying/linking it into the models dir.
    std::optional<ModelInfo> import(const std::string& srcPath, std::string* err);

    const std::string& dir() const { return dir_; }

private:
    std::string dir_;
};

} // namespace trout
