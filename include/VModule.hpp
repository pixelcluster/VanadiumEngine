#pragma once

#include <string_view>
#include <unordered_map>
#include <variant>
#include <optional>

class VEngine;
class VModule;

template <typename Derived, typename Base>
concept DerivesFrom = requires(Derived* derived) {
	static_cast<Base*>(derived);
};

struct VResource {
	void* data;
	size_t size;
};

enum class VInputOutputVariableType { Int, Float, Bool, String, Resource };

struct VInputOutputVariable {
	VInputOutputVariableType type;
	std::variant<int, float, bool, std::string, VResource> data;
};

struct VInputVariableReference {
	std::string inputVariableName;
	VModule* outputModule;
	std::string outputVariableName;
};

class VModuleGroup {
  public:
	virtual ~VModuleGroup() = 0;

	virtual void onCreate(VEngine& engine) = 0;
	virtual void onModuleAdd(VEngine& engine, VModule* addedModule) = 0;

	virtual void onModuleRemove(VEngine& engine, VModule* removedModule) = 0;
	virtual void onDestroy(VEngine& engine) = 0;

	const std::vector<VModule*>& modules() const { return m_modules; }

  private:
	std::vector<VModule*> m_modules;
	friend class VEngine;
};


class VModule {
  public:
	virtual ~VModule() {}

	virtual void onCreate(VEngine& engine) = 0;
	virtual void onActivate(VEngine& engine) = 0;

	virtual void onExecute(VEngine& engine,
						   const std::unordered_map<std::string, const VInputOutputVariable*>& inputVariables) = 0;

	virtual void onDeactivate(VEngine& engine) = 0;
	virtual void onDestroy(VEngine& engine) = 0;

	std::optional<const VInputOutputVariable*> outputVariable(const std::string_view& name) const;

	void addInputVariable(const VInputVariableReference& inputVariableReference);

	const std::vector<VInputVariableReference>& inputVariables() const { return m_inputVariableReferences; }

	void removeInputVariable(size_t index);
	void removeInputVariable(const std::string_view& name);

  protected:
	void initializeTypedOutput(const std::string_view& name, VInputOutputVariableType type);

	void writeOutput(const std::string_view& name, int value);
	void writeOutput(const std::string_view& name, float value);
	void writeOutput(const std::string_view& name, bool value);
	void writeOutput(const std::string_view& name, const std::string& value);
	void writeOutput(const std::string_view& name, const VResource& value);

	VModuleGroup* m_moduleGroup = nullptr;

	friend class VEngine;
  private:
	std::unordered_map<std::string, VInputOutputVariable> m_outputVariables;
	std::vector<VInputVariableReference> m_inputVariableReferences;
};