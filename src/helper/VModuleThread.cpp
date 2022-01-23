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


	for (auto& variable : info.executedModule->inputVariables()) {
		auto outputVariable = variable.outputModule->outputVariable(variable.outputVariableName);
		if (outputVariable)
			info.executedModule->setInputVariable(variable.inputVariableName, *outputVariable);
	}

	info.executedModule->onExecute(engine);

	if (info.signalAtomic.has_value()) {
		info.signalAtomic.value() = true;
	}
}
