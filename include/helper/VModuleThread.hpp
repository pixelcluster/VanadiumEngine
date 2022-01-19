#pragma once

#include <helper/VModuleThreadingInfo.h>

struct VModuleThreadParameters {
	std::vector<VModuleExecutionInfo>& executionInfo;
	VEngine& engine;
	std::atomic<bool> runFlag;
	std::atomic<bool> exitFlag;
};

void moduleThreadFunction(VModuleThreadParameters& parameters);

void executeModule(VModuleExecutionInfo& info, VEngine& engine);