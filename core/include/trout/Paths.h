#pragma once

#include <string>

namespace trout {

// Resolves on-disk locations under the user's Application Support directory.
// All trout data lives under ~/Library/Application Support/app.trout.Trout by
// default. The macOS layer may override the root for testing.
class Paths {
public:
    static Paths& instance();

    void setRoot(const std::string& root);
    const std::string& root() const { return root_; }

    std::string databaseFile() const;   // settings + profiles + stats
    std::string modelsDir() const;       // downloaded/imported GGUF models
    std::string logFile() const;
    std::string diagnosticsDir() const;

    // Ensure all directories exist. Returns false on failure.
    bool ensureDirectories() const;

private:
    Paths();
    std::string root_;
};

} // namespace trout
