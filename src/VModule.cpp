#include <VModule.hpp>


const VModuleOutputData* VModule::outputVariable(const std::string_view& name) const {
	auto iterator = m_outputData.find(std::string(name));
	if (iterator == m_outputData.end())
		return nullptr;
	else {
		return &iterator->second;
	}
}

void VModule::setInputVariable(const std::string& name, const VModuleOutputData& value) {
	m_inputVariableValues[name] = value;
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

void VModule::setOptionVariable(const std::string_view& name, const VOptionVariable& value) {
	m_optionVariables.insert({ std::string(name), value });
}

void VModule::removeOptionVariable(const std::string& name) { m_optionVariables.erase(name); }

void VModule::removeReferencedInputs(VModule* moduleToRemove) {
	for (size_t i = 0; i < m_inputVariableReferences.size(); ++i) {
		if (m_inputVariableReferences[i].outputModule == moduleToRemove) {
			
		}
	}
}
