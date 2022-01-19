#pragma once

#include <helper/VModuleThreadingInfo.h>

void singleThreadScoreboard(const std::vector<VModule*>& activatedModules,
							const std::vector<VModuleDependency>& moduleDependencies,
							std::vector<std::vector<VModuleExecutionInfo>>& threadExecutionInfos);