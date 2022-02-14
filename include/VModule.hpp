#pragma once

#include <string_view>
#include <unordered_map>
#include <variant>
#include <optional>

class VEngine;
class VModule;

enum class VOptionVariableType { Int, Float, Bool, String };

struct VOptionVariable {
	VOptionVariableType type;
	std::variant<int, float, bool, std::string> data;
};

struct VInputVariableReference {
	std::string inputVariableName;
	VModule* outputModule;
	std::string outputVariableName;
};

struct VModuleOutputData {
	const void* value;
	size_t typeHash;
};

class VModule {
  public:
	virtual ~VModule() {}

	virtual void onCreate(VEngine& engine) = 0;
	virtual void onActivate(VEngine& engine) = 0;

	virtual void onExecute(VEngine& engine) = 0;

	virtual void onDeactivate(VEngine& engine) = 0;
	virtual void onDestroy(VEngine& engine) = 0;

	virtual void onDependentModuleDeactivate(VEngine& engine, VModule* moduleToDestroy) {}

	const std::vector<VInputVariableReference>& inputVariables() const { return m_inputVariableReferences; }
	const std::unordered_map<std::string, VModuleOutputData>& outputVariables() const { return m_outputData; }

	const VModuleOutputData* outputVariable(const std::string_view& name) const;
	void setInputVariable(const std::string& name, const VModuleOutputData& value);

	void setOptionVariable(const std::string_view& name, const VOptionVariable& value);
	void removeOptionVariable(const std::string& name);

	void addInputVariable(const VInputVariableReference& inputVariableReference);

	void removeInputVariable(size_t index);
	void removeInputVariable(const std::string_view& name);
	void removeReferencedInputs(VModule* moduleToRemove);

  protected:
	template <typename T> const T* retrieveInputVariable(const std::string& name);

	template <typename T> void addOutputVariable(const std::string_view& name);

	template <typename T> void setOutputVariable(const std::string_view& name, const T* value);

	std::unordered_map<std::string, VOptionVariable> m_optionVariables;
	friend class VEngine;
  private:
	std::vector<VInputVariableReference> m_inputVariableReferences;
	std::unordered_map<std::string, VModuleOutputData> m_inputVariableValues;
	std::unordered_map<std::string, VModuleOutputData> m_outputData;
};

template <typename T> inline const T* VModule::retrieveInputVariable(const std::string& name) {
	constexpr size_t hash = typeid(T).hash_code();
	auto iterator = m_inputVariableValues.find(name);
	if (iterator != m_inputVariableValues.end()) {
		if (iterator->second.typeHash != hash) {
			// TODO: log setting an output variable to an invalid type
		} else {
			return reinterpret_cast<const T*>(iterator->second.value);
		}
	}
	return nullptr;
}

template <typename T> inline void VModule::addOutputVariable(const std::string_view& name) {
	constexpr size_t hash = typeid(T).hash_code();
	m_outputData.insert(std::pair<std::string, VModuleOutputData>(name, {nullptr, hash }));
}

template <typename T> inline void VModule::setOutputVariable(const std::string_view& name, const T* value) {
	auto variableIterator = m_outputData.find(std::string(name));
	constexpr size_t hash = typeid(T).hash_code();
	if (variableIterator != m_outputData.end()) {
		if (variableIterator->second.typeHash != hash) {
			//TODO: log setting an output variable to an invalid type
		}
		else {
			variableIterator->second.value = value;
		}
	}
}
