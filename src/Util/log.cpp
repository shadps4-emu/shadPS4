#include <vector>
#include <spdlog/common.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/pattern_formatter.h>
#include <spdlog/spdlog.h>



namespace logging {
	std::vector<spdlog::sink_ptr> sinks;

	int init(bool use_stdout) {
        auto f = std::make_unique<spdlog::pattern_formatter>("%^|%L|: %v%$", spdlog::pattern_time_type::local, std::string(""));  // disable eol
        spdlog::set_formatter(std::move(f));
		sinks.clear();//clear existing sinks
		if (use_stdout)//if we use stdout window then init it as well
			sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());

		return 0;//all ok
	}
}