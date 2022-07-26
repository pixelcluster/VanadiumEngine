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

#include <vector>
#ifdef __linux__
#include <unistd.h>

struct SubprocessID {
	pid_t childPID;
};

#elif defined(_WIN32)
#define NOMINMAX
#include <Windows.h>

struct SubprocessID {
	HANDLE processHandle;
	HANDLE threadHandle;
};
#endif

SubprocessID startSubprocess(const char* processName, const std::vector<const char*>& arguments);
void waitForSubprocess(const SubprocessID& id);
