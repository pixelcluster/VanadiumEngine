#include <helper/VScoreboarding.hpp>
#include <algorithm>

void singleThreadScoreboard(const std::vector<VModule*>& activatedModules,
							const std::vector<VModuleDependency>& moduleDependencies,
							std::vector<std::vector<VModuleExecutionInfo>>& threadExecutionInfos) {
	if (threadExecutionInfos.empty()) {
		threadExecutionInfos.resize(1);
	}

	threadExecutionInfos[0].clear();
	threadExecutionInfos[0].reserve(activatedModules.size());

	for (auto& activatedModule : activatedModules) {
		threadExecutionInfos[0].push_back({ .executedModule = activatedModule });
	}

	std::sort(threadExecutionInfos[0].begin(), threadExecutionInfos[0].end(),
			  [&moduleDependencies](const auto& oneObject, const auto& otherObject) {
				  for (auto& dependency : moduleDependencies) {
					  if (dependency.from == oneObject.executedModule && dependency.to == otherObject.executedModule)
						  return true;
				  }
				  return false;
			  });
}