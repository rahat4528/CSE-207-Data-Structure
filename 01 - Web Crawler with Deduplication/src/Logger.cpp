#include "Logger.h"
#include <iomanip>
#include <thread>

namespace crawler {

void Logger::log(LogLevel level, std::string_view msg) {
    if (level < m_level) return;

    auto now = std::chrono::system_clock::now();
    auto tt = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::tm tmBuf{};
#if defined(_WIN32)
    gmtime_s(&tmBuf, &tt);
#else
    gmtime_r(&tt, &tmBuf);
#endif

    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_out) return;

    (*m_out) << "[" << std::put_time(&tmBuf, "%Y-%m-%d %H:%M:%S")
             << "." << std::setfill('0') << std::setw(3) << ms.count()
             << "] [" << logLevelToString(level) << "] "
             << msg << "\n";
    m_out->flush();
}

} // namespace crawler

