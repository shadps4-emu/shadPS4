#include <spdlog/common.h>
#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <vector>
#include <Util/config.h>

namespace logging {
std::vector<spdlog::sink_ptr> sinks;

int init(bool use_stdout) {
    sinks.clear();   // clear existing sinks
    if (use_stdout)  // if we use stdout window then init it as well
        sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
    sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(L"shadps4.txt", true));
    spdlog::set_default_logger(std::make_shared<spdlog::logger>("shadps4 logger", begin(sinks), end(sinks)));
    auto f = std::make_unique<spdlog::pattern_formatter>("%^|%L|: %v%$", spdlog::pattern_time_type::local, std::string(""));  // disable eol
    spdlog::set_formatter(std::move(f));
    spdlog::set_level(static_cast<spdlog::level::level_enum>(Config::getLogLevel()));
    spdlog::level::level_enum t = spdlog::get_level();
    return 0;  // all ok
}

void set_level(spdlog::level::level_enum log_level) { spdlog::set_level(log_level); }

}  // namespace logging