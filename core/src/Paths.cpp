#include "trout/Paths.h"

#include <cstdlib>
#include <filesystem>

namespace fs = std::filesystem;

namespace trout {

Paths& Paths::instance() {
    static Paths p;
    return p;
}

Paths::Paths() {
    const char* home = std::getenv("HOME");
    std::string base = home ? home : ".";
    root_ = base + "/Library/Application Support/app.trout.Trout";
}

void Paths::setRoot(const std::string& root) { root_ = root; }

std::string Paths::databaseFile() const { return root_ + "/trout.sqlite3"; }
std::string Paths::modelsDir() const { return root_ + "/models"; }
std::string Paths::logFile() const { return root_ + "/trout.log"; }
std::string Paths::diagnosticsDir() const { return root_ + "/diagnostics"; }

bool Paths::ensureDirectories() const {
    std::error_code ec;
    fs::create_directories(root_, ec);
    fs::create_directories(modelsDir(), ec);
    fs::create_directories(diagnosticsDir(), ec);
    return !ec;
}

} // namespace trout
