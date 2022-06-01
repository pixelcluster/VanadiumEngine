#pragma once
#include <Debug.hpp>
#include <fmt/core.h>

namespace vanadium {
	template <typename... Ts> void logInfo(const char* message, Ts... values) {
		if constexpr (vanadiumDebug) {
			fmt::print(fmt::runtime(message), values...);
			fmt::print("\n");
		}
	}

	template <typename... Ts> void logWarning(const char* message, Ts... values) {
		fmt::print(fmt::runtime(message), values...);
		fmt::print("\n");
	}

	template <typename... Ts> void logError(const char* message, Ts... values) {
		fmt::print(stderr, fmt::runtime(message), values...);
		fmt::print(stderr, "\n");
	}

	template <typename... Ts> void logFatal(const char* message, Ts... values) {
		fmt::print(stderr, fmt::runtime(message), values...);
		fmt::print(stderr, "\n");
		std::abort();
	}

	template <typename... Ts> void logFatal(int exitCode, const char* message, Ts... values) {
		fmt::print(stderr, fmt::runtime(message), values...);
		fmt::print(stderr, "\n");
		std::exit(exitCode);
	}

	inline void assertFatal(bool value, const char* message) {
		if (!value) {
			logFatal(message);
		}
	}
} // namespace vanadium