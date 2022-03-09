#pragma once

#include <vector>
#ifdef __linux__
#include <unistd.h>

struct SubprocessID {
	pid_t childPID;
};

#elif defined(_WIN32)
#include <Windows.h>

struct SubprocessID {
	HANDLE processHandle;
	HANDLE threadHandle;
};
#endif

SubprocessID startSubprocess(char* processName, std::vector<char*> arguments);
void waitForSubprocess(const SubprocessID& id);