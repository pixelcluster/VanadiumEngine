#include <VModule.hpp>

std::optional<const VInputOutputVariable&> VModule::outputVariable(const std::string_view& name) const {
	auto iterator = m_outputVariables.find(std::string(name));
	if (iterator == m_outputVariables.end())
		return std::nullopt;
	else {
		return iterator->second;
	}
}

void VModule::addInputVariable(const VInputVariableReference& inputVariableReference) {
	m_inputVariableReferences.push_back(inputVariableReference);
}

void VModule::removeInputVariable(size_t index) {
	m_inputVariableReferences.erase(m_inputVariableReferences.begin() + index);
}

void VModule::removeInputVariable(const std::string_view& name) {
	auto iterator = std::find_if(m_inputVariableReferences.begin(), m_inputVariableReferences.end(),
								 [&name](const auto& object) { return object.inputVariableName == name; });
	if (iterator != m_inputVariableReferences.end())
		m_inputVariableReferences.erase(iterator);
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

void VModule::writeOutput(const std::string_view& name, const VResource& value) {
	auto iterator = m_outputVariables.find(std::string(name));
	if (iterator == m_outputVariables.end()) {
		// TODO: log attempt to set undeclared output variable
	} else if (iterator->second.type != VInputOutputVariableType::Resource) {
		// TODO: log attempt to set wrong output variable type
	} else {
		iterator->second.data = value;
	}
}
