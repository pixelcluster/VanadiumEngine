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
#include <SysUtils.hpp>
#include <iostream>
#include <cstring>

#ifdef __linux__
#include <sys/types.h>
#include <sys/wait.h>

SubprocessID startSubprocess(const char* processName, const std::vector<const char*>& arguments) { 
	size_t totalArgsSize = strlen(processName) + 1;
	for (auto& arg : arguments) {
		totalArgsSize += strlen(arg) + 1;
	}

	char* argData = new char[totalArgsSize];
	char** argArray = new char*[arguments.size() + 2];

	argArray[0] = argData;
	argArray[arguments.size() + 1] = nullptr;
	
	size_t argOffset = strlen(processName) + 1;
	size_t argIndex = 1;

	std::memcpy(argData, processName, argOffset);
	for (auto& arg : arguments) {
		size_t argSize = strlen(arg) + 1;
		std::memcpy(argData + argOffset, arg, argSize);
		argArray[argIndex] = argData + argOffset;
		argOffset += argSize;
		++argIndex;
	}


	pid_t childPID = fork();
	switch (childPID) {
		case 0:
			execvp(processName, argArray);
			break;
		case -1:
			std::cout << "Error: Failed to open subprocess, exiting!" << std::endl;
			std::exit(EXIT_FAILURE);
			break;
		default:
			delete[] argData;
			delete[] argArray;
			return { childPID };
	}
	return { -1 }; // unreachable but silences compiler warning
}

void waitForSubprocess(const SubprocessID& id) { 
	int status;
	waitpid(id.childPID, &status, 0);
}

#elif defined(_WIN32)

SubprocessID startSubprocess(const char* processName, const std::vector<const char*>& arguments) {
	size_t commandLineLength = strlen(processName);
	for (auto& argument : arguments) {
		//space plus argument size
		commandLineLength += 1U + strlen(argument);
	}
	//terminator byte
	commandLineLength++;
	char* commandLine = new char[commandLineLength];

	size_t commandLineOffset = 0;
	size_t currentArgLength = strlen(processName);
	std::memcpy(commandLine, processName, currentArgLength);
	commandLineOffset += currentArgLength;
	for (auto& argument : arguments) {
		commandLine[commandLineOffset] = ' ';
		++commandLineOffset;

		currentArgLength = strlen(argument);
		std::memcpy(commandLine + commandLineOffset, argument, currentArgLength);
		commandLineOffset += currentArgLength;
	}
	commandLine[commandLineLength - 1] = 0;

	STARTUPINFOA startupInfo = {};
	PROCESS_INFORMATION processInfo = {};

	if (!CreateProcessA(nullptr, commandLine, nullptr, nullptr, FALSE, NORMAL_PRIORITY_CLASS, nullptr, nullptr,
						&startupInfo, &processInfo)) {
		std::cout << "Error: Failed to open subprocess, exiting!" << std::endl;
		delete[] commandLine;
		std::exit(EXIT_FAILURE);
	}

	delete[] commandLine;
	return { .processHandle = processInfo.hProcess, .threadHandle = processInfo.hThread };
}

void waitForSubprocess(const SubprocessID& id) { 
	WaitForSingleObject(id.processHandle, INFINITE);
	CloseHandle(id.processHandle);
	CloseHandle(id.threadHandle);
}
#else
#error Unsupported platform encountered, currently only supporting Windows and Linux.
#endif
