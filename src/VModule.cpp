#include <VModule.hpp>

const VInputOutputVariable& VModule::outputVariable(const std::string_view& name) const {
	auto iterator = m_outputVariables.find(std::string(name));
	if (iterator == m_outputVariables.end())
		return {};
	else {
		return iterator->second;
	}
}

void VModule::initializeTypedOutput(const std::string_view& name, VInputOutputVariableType variableType) {
	m_outputVariables.insert(
		std::pair<std::string, VInputOutputVariable>(std::string(name), VInputOutputVariable{ .type = variableType }));
}

void VModule::writeOutput(const std::string_view& name, int value) {
	auto iterator = m_outputVariables.find(std::string(name));
	if (iterator == m_outputVariables.end()) {
		// TODO: log attempt to set undeclared output variable
	} else if (iterator->second.type != VInputOutputVariableType::Int) {
		// TODO: log attempt to set wrong output variable type
	} else {
		iterator->second.data = value;
	}
}

void VModule::writeOutput(const std::string_view& name, float value) {
	auto iterator = m_outputVariables.find(std::string(name));
	if (iterator == m_outputVariables.end()) {
		//TODO: log attempt to set undeclared output variable
	} else if (iterator->second.type != VInputOutputVariableType::Float) {
		// TODO: log attempt to set wrong output variable type
	} 
	else {
		iterator->second.data = value;
	}
}

void VModule::writeOutput(const std::string_view& name, bool value) {
	auto iterator = m_outputVariables.find(std::string(name));
	if (iterator == m_outputVariables.end()) {
		// TODO: log attempt to set undeclared output variable
	} else if (iterator->second.type != VInputOutputVariableType::Bool) {
		// TODO: log attempt to set wrong output variable type
	} else {
		iterator->second.data = value;
	}
}

void VModule::writeOutput(const std::string_view& name, const std::string& value) {
	auto iterator = m_outputVariables.find(std::string(name));
	if (iterator == m_outputVariables.end()) {
		// TODO: log attempt to set undeclared output variable
	} else if (iterator->second.type != VInputOutputVariableType::String) {
		// TODO: log attempt to set wrong output variable type
	} else {
		iterator->second.data = value;
	}
}

void VModule::writeOutput(const std::string_view& name, const VCPUResource& value) {
	auto iterator = m_outputVariables.find(std::string(name));
	if (iterator == m_outputVariables.end()) {
		// TODO: log attempt to set undeclared output variable
	} else if (iterator->second.type != VInputOutputVariableType::CPUResource) {
		// TODO: log attempt to set wrong output variable type
	} else {
		iterator->second.data = value;
	}
}

void VModule::writeOutput(const std::string_view& name, const gpu::VGPUResource& value) {
	auto iterator = m_outputVariables.find(std::string(name));
	if (iterator == m_outputVariables.end()) {
		// TODO: log attempt to set undeclared output variable
	} else if (iterator->second.type != VInputOutputVariableType::GPUResource) {
		// TODO: log attempt to set wrong output variable type
	} else {
		iterator->second.data = value;
	}
}
