#include <map>
#include <string>

#include "types.h"

#include <Util/log.h>

namespace aerolib {
	#define STUB(a, b) { a, #b },
	std::map<std::string, std::string> symbolsMap = {
		#include "stubs.h"
	};
	#undef STUB

	#define STUB(a, b) \
        uint64_t PS4_SYSV_ABI stub_##b() { \
			LOG_ERROR_IF(true, #b " STUB!\n"); \
			 \
			return 0; \
		}
	#include "stubs.h"
	#undef STUB

	#define STUB(a, b) {a, (uint64_t) & stub_##b},
		std::map<std::string, uint64_t> symbolsStubsMap = {
			#include "stubs.h"
		};
	#undef STUB
    };