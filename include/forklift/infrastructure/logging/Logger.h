// Minimal structured logger. Replace with spdlog/glog at integration time —
// the call sites (LOG_INFO, LOG_WARN, LOG_ERROR) are the public contract.

#ifndef FORKLIFT_INFRASTRUCTURE_LOGGING_LOGGER_H_
#define FORKLIFT_INFRASTRUCTURE_LOGGING_LOGGER_H_

#include <sstream>
#include <string>
#include <string_view>

namespace forklift::infrastructure::logging {

enum class Level { kDebug = 0, kInfo = 1, kWarn = 2, kError = 3 };

void  set_level(Level lvl);
Level get_level();

void log(Level lvl, std::string_view file, int line, std::string_view message);

}  // namespace forklift::infrastructure::logging

#define FSS_LOG(LVL, MSG_EXPR)                                                                  \
    do {                                                                                        \
        if (::forklift::infrastructure::logging::get_level() <= (LVL)) {                        \
            std::ostringstream _fss_oss;                                                        \
            _fss_oss << MSG_EXPR;                                                               \
            ::forklift::infrastructure::logging::log((LVL), __FILE__, __LINE__, _fss_oss.str());\
        }                                                                                       \
    } while (0)

#define LOG_DEBUG(MSG) FSS_LOG(::forklift::infrastructure::logging::Level::kDebug, MSG)
#define LOG_INFO(MSG)  FSS_LOG(::forklift::infrastructure::logging::Level::kInfo,  MSG)
#define LOG_WARN(MSG)  FSS_LOG(::forklift::infrastructure::logging::Level::kWarn,  MSG)
#define LOG_ERROR(MSG) FSS_LOG(::forklift::infrastructure::logging::Level::kError, MSG)

#endif  // FORKLIFT_INFRASTRUCTURE_LOGGING_LOGGER_H_
