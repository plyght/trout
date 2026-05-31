#pragma once

#include <string>
#include <sstream>
#include <mutex>
#include <fstream>

namespace trout {

enum class LogLevel { Debug, Info, Warn, Error };

// Privacy-conscious logger: by default it must not store raw user text. Call
// sites that handle user content should redact before logging.
class Log {
public:
    static Log& instance();

    void setLevel(LogLevel level);
    void setLogFile(const std::string& path);
    void write(LogLevel level, const std::string& msg);

private:
    Log() = default;
    std::mutex mutex_;
    LogLevel level_ = LogLevel::Info;
    std::ofstream file_;
    bool toStderr_ = true;
};

#define TROUT_LOG(level, expr)                                  \
    do {                                                        \
        std::ostringstream _oss;                                \
        _oss << expr;                                           \
        ::trout::Log::instance().write(level, _oss.str());      \
    } while (0)

#define TROUT_DEBUG(expr) TROUT_LOG(::trout::LogLevel::Debug, expr)
#define TROUT_INFO(expr)  TROUT_LOG(::trout::LogLevel::Info, expr)
#define TROUT_WARN(expr)  TROUT_LOG(::trout::LogLevel::Warn, expr)
#define TROUT_ERROR(expr) TROUT_LOG(::trout::LogLevel::Error, expr)

} // namespace trout
