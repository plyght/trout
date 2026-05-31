#include "trout/Log.h"

#include <iostream>
#include <ctime>

namespace trout {

Log& Log::instance() {
    static Log log;
    return log;
}

void Log::setLevel(LogLevel level) {
    std::lock_guard<std::mutex> lk(mutex_);
    level_ = level;
}

void Log::setLogFile(const std::string& path) {
    std::lock_guard<std::mutex> lk(mutex_);
    file_.open(path, std::ios::app);
}

static const char* levelName(LogLevel l) {
    switch (l) {
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info:  return "INFO";
        case LogLevel::Warn:  return "WARN";
        case LogLevel::Error: return "ERROR";
    }
    return "?";
}

void Log::write(LogLevel level, const std::string& msg) {
    std::lock_guard<std::mutex> lk(mutex_);
    if (static_cast<int>(level) < static_cast<int>(level_)) return;

    std::time_t t = std::time(nullptr);
    char ts[32];
    std::strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", std::localtime(&t));

    std::string line = std::string("[") + ts + "] " + levelName(level) + " " + msg + "\n";
    if (toStderr_) std::cerr << line;
    if (file_.is_open()) { file_ << line; file_.flush(); }
}

} // namespace trout
