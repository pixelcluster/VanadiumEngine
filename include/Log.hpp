#pragma once
#include <Debug.hpp>
#include <cstdio>
#include <cstdlib>

namespace vanadium {
	template <typename... Ts> void logInfo(const char* message, Ts... values) {
		if constexpr (vanadiumDebug) {
			fprintf(stdout, message, values...);
		}
	}

	template <typename... Ts> void logWarning(const char* message, Ts... values) {
		fprintf(stdout, message, values...);
	}

	template <typename... Ts> void logError(const char* message, Ts... values) { fprintf(stderr, message, values...); }

	template <typename... Ts> void logFatal(const char* message, Ts... values) { 
        fprintf(stderr, message, values...);
        std::abort();
    }

	template <typename... Ts> void logFatal(int exitCode, const char* message, Ts... values) { 
        fprintf(stderr, message, values...);
        std::exit(exitCode);
    }

    inline void assertFatal(bool value, const char* message) {
        if(!value) {
            logFatal(message);
        }
    }
} // namespace vanadium