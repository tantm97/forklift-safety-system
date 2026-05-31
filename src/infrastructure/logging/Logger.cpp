#include "forklift/infrastructure/logging/Logger.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <mutex>

namespace forklift::infrastructure::logging {

namespace {

std::atomic<Level> g_level{Level::kInfo};
std::mutex         g_io_mu;

const char* tag(Level l) {
    switch (l) {
        case Level::kDebug: return "DEBUG";
        case Level::kInfo:  return "INFO ";
        case Level::kWarn:  return "WARN ";
        case Level::kError: return "ERROR";
    }
    return "?????";
}

}  // namespace

void  set_level(Level lvl) { g_level.store(lvl); }
Level get_level()          { return g_level.load(); }

void log(Level lvl, std::string_view file, int line, std::string_view message) {
    using namespace std::chrono;
    const auto now    = system_clock::now();
    const auto t      = system_clock::to_time_t(now);
    const auto ms     = duration_cast<milliseconds>(now.time_since_epoch()).count() % 1000;
    std::tm tm{};
#if defined(_WIN32)
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    std::lock_guard<std::mutex> lock(g_io_mu);
    std::ostream& out = (lvl >= Level::kWarn) ? std::cerr : std::cout;
    out << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S")
        << '.' << std::setw(3) << std::setfill('0') << ms << "Z "
        << '[' << tag(lvl) << "] "
        << file << ':' << line << "  "
        << message << '\n';
}

}  // namespace forklift::infrastructure::logging
