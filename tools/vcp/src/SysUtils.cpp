#include <SysUtils.hpp>
#include <iostream>

#ifdef __linux__
#include <sys/types.h>
#include <sys/wait.h>

SubprocessID startSubprocess(char* processName, std::vector<char*> arguments) { 
	arguments.insert(arguments.begin(), processName);
	arguments.push_back(nullptr);

	pid_t childPID = fork();
	switch (childPID) {
		case 0:
			execvp(processName, arguments.data());
			break;
		case -1:
			std::cout << "Error: Failed to open subprocess, exiting!" << std::endl;
			std::exit(EXIT_FAILURE);
			break;
		default:
			return { childPID };
	}
	return { -1 }; // unreachable but silences compiler warning
}

void waitForSubprocess(const SubprocessID& id) { 
	int status;
	waitpid(id.childPID, &status, 0);
}

#elif defined(_WIN32)

SubprocessID startSubprocess(char* processName, std::vector<char*> arguments) {
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
		std::exit(EXIT_FAILURE);
	}

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