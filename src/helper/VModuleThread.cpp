#include <helper/VModuleThread.hpp>
#include <atomic>

void moduleThreadFunction(VModuleThreadParameters* parameters) {
	parameters->runFlag.value.wait(false);
	while (!parameters->exitFlag.value) {
		for (auto& executionInfo : *parameters->executionInfo) {
			executeModule(executionInfo, *parameters->engine);
		}

		parameters->runFlag = false;
		parameters->runFlag.value.wait(false);
	}
}

void executeModule(VModuleExecutionInfo& info, VEngine& engine) {
	for (auto& waitAtomic : info.waitAtomics) {
		waitAtomic->wait(false);
	}

	std::unordered_map<std::string, const VInputOutputVariable*> moduleInputVariables;
	moduleInputVariables.reserve(info.executedModule->inputVariables().size());

	for (auto& variable : info.executedModule->inputVariables()) {
		auto outputVariable = variable.outputModule->outputVariable(variable.outputVariableName);
		if (outputVariable.has_value())
			moduleInputVariables.insert(
				std::pair<std::string, const VInputOutputVariable*>(variable.inputVariableName, outputVariable.value()));
	}

	info.executedModule->onExecute(engine, moduleInputVariables);

	if (info.signalAtomic.has_value()) {
		info.signalAtomic.value() = true;
	}
}
