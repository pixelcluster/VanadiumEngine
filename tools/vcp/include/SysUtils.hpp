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