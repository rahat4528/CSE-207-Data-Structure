#ifndef CRAWLER_LOGGER_H
#define CRAWLER_LOGGER_H

#include <string>
#include <string_view>
#include <mutex>
#include <iostream>
#include <sstream>
#include <chrono>

namespace crawler {

enum class LogLevel {
    DEBUG = 0,
    INFO,
    WARN,
    ERROR_LEVEL,
    NONE
};

inline std::string_view logLevelToString(LogLevel level) noexcept {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO";
        case LogLevel::WARN:  return "WARN";
        case LogLevel::ERROR_LEVEL: return "ERROR";
        default: return "NONE";
    }
}

class Logger {
public:
    static Logger& instance() {
        static Logger inst;
        return inst;
    }

    void setLevel(LogLevel level) noexcept {
        m_level = level;
    }

    LogLevel level() const noexcept {
        return m_level;
    }

    void setOutput(std::ostream* out) noexcept {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_out = out;
    }

    void log(LogLevel level, std::string_view msg);

    void debug(std::string_view msg) { log(LogLevel::DEBUG, msg); }
    void info(std::string_view msg)  { log(LogLevel::INFO, msg); }
    void warn(std::string_view msg)  { log(LogLevel::WARN, msg); }
    void error(std::string_view msg) { log(LogLevel::ERROR_LEVEL, msg); }

private:
    Logger() : m_level(LogLevel::INFO), m_out(&std::cout) {}
    ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    LogLevel m_level;
    std::ostream* m_out;
    std::mutex m_mutex;
};

#define LOG_DEBUG(msg) do { std::ostringstream _oss; _oss << msg; crawler::Logger::instance().debug(_oss.str()); } while(0)
#define LOG_INFO(msg)  do { std::ostringstream _oss; _oss << msg; crawler::Logger::instance().info(_oss.str());  } while(0)
#define LOG_WARN(msg)  do { std::ostringstream _oss; _oss << msg; crawler::Logger::instance().warn(_oss.str());  } while(0)
#define LOG_ERROR(msg) do { std::ostringstream _oss; _oss << msg; crawler::Logger::instance().error(_oss.str()); } while(0)

} // namespace crawler

#endif // CRAWLER_LOGGER_H

