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
#include <cstdio>
#include <fmt/color.h>
#include <fmt/core.h>
#include <time.h>

namespace vanadium {

	enum class SubsystemID { Unknown, RHI };

	inline const char* subsystemName(SubsystemID id) {
		switch (id) {
			case SubsystemID::RHI:
				return "[RHI] ";
			default:
				return "";
		}
	}

	inline void logTime(FILE* file) {
		time_t currentTime;
		tm* localTime;
		time(&currentTime);
		localTime = localtime(&currentTime);

		fmt::print(file, "[ {}:{}:{} ] ", localTime->tm_hour, localTime->tm_min, localTime->tm_sec);
	}

	template <typename... Ts> void logInfo(SubsystemID id, const char* message, Ts... values) {
		if constexpr (vanadiumDebug) {
			logTime(stdout);
			fmt::print(fmt::fg(fmt::color::gray), "INFO ");
			fmt::print(fmt::fg(fmt::color::gray), subsystemName(id));
			fmt::print(fmt::runtime(message), values...);
			fmt::print("\n");
		}
	}

	template <typename... Ts> void logWarning(SubsystemID id, const char* message, Ts... values) {
		logTime(stdout);
		fmt::print(fmt::fg(fmt::color::yellow), "WARN ");
		fmt::print(fmt::fg(fmt::color::yellow), subsystemName(id));
		fmt::print(fmt::runtime(message), values...);
		fmt::print("\n");
	}

	template <typename... Ts> void logError(SubsystemID id, const char* message, Ts... values) {
		logTime(stderr);
		fmt::print(stderr, fmt::fg(fmt::color::red), "ERROR ");
		fmt::print(fmt::fg(fmt::color::red), subsystemName(id));
		fmt::print(stderr, fmt::runtime(message), values...);
		fmt::print(stderr, "\n");
	}

	template <typename... Ts> void logFatal(SubsystemID id, const char* message, Ts... values) {
		logTime(stderr);
		fmt::print(stderr, fmt::fg(fmt::color::dark_red), "FATAL ");
		fmt::print(fmt::fg(fmt::color::dark_red), subsystemName(id));
		fmt::print(stderr, fmt::runtime(message), values...);
		fmt::print(stderr, "");
		std::abort();
	}

	template <typename... Ts> void logFatal(int exitCode, SubsystemID id, const char* message, Ts... values) {
		logTime(stderr);
		fmt::print(stderr, fmt::fg(fmt::color::dark_red), "FATAL ");
		fmt::print(fmt::fg(fmt::color::dark_red), subsystemName(id));
		fmt::print(stderr, fmt::runtime(message), values...);
		fmt::print(stderr, "\n");
		std::exit(exitCode);
	}

	inline void assertFatal(bool value, SubsystemID id, const char* message) {
		if (!value) {
			logFatal(id, message);
		}
	}
} // namespace vanadium
