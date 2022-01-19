#include <HelloWorld.hpp>
#include <iostream>

void HelloWorldModule::onCreate(VEngine& engine) {
	std::cout << "onCreate called!\n";
}

void HelloWorldModule::onActivate(VEngine& engine) { std::cout << "onActivate called!\n"; }

void HelloWorldModule::onExecute(
	VEngine& engine,
			   const std::unordered_map<std::string, const VInputOutputVariable*>& inputVariables) {
	std::cout << "onExecute called! Previous executions: " << m_nExecutions++ << "\n";
	if (m_nExecutions >= 5) {
		engine.setExitFlag();
	}
}

void HelloWorldModule::onDeactivate(VEngine& engine) { std::cout << "onDeactivate called!\n"; }

void HelloWorldModule::onDestroy(VEngine& engine) { std::cout << "onDestroy called!\n"; }

void main() { 
	VEngine engine;
	HelloWorldModule* helloModule = engine.createModule<HelloWorldModule>();
	engine.activateModule(helloModule);
	engine.run();
}