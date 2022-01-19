#pragma once

#include <VModule.hpp>
#include <atomic>
#include <optional>


struct VModuleDependency {
	VModule* from;
	VModule* to;
};

struct VModuleExecutionInfo {
	std::vector<std::atomic<bool>*> waitAtomics;
	VModule* executedModule;
	std::optional<std::atomic<bool>> signalAtomic;
};