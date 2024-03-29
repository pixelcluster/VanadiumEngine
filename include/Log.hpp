/* VanadiumEngine, a Vulkan rendering toolkit
 * Copyright (C) 2022 Friedrich Vock
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
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
		fmt::print(stderr, "");
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
