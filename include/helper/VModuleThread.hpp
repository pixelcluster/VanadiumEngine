#pragma once

#include <helper/VModuleThreadingInfo.h>

struct VModuleThreadParameters {
	std::vector<VModuleExecutionInfo>* executionInfo;
	VEngine* engine;
	AtomicBoolWrapper runFlag;
	AtomicBoolWrapper exitFlag;
};

void moduleThreadFunction(VModuleThreadParameters* parameters);

void executeModule(VModuleExecutionInfo& info, VEngine& engine);