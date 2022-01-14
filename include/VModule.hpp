#pragma once

#include <string_view>
#include <unordered_map>

class VEngine;

template<typename T>
struct VModuleVariableTemplate {
	std::string name;
};

//placeholder, may want to include typeid of variable in debug for type safety-checking
struct VModuleVariable {
	void* data;
};

struct VModuleVariableContainer {
  public:
	template<typename... Ts>
	VModuleVariableContainer(const VModuleVariableTemplate<Ts> templates...);
	~VModuleVariableContainer();

  private:
	  template<typename T> void allocateVariableTemplate(const VModuleVariableTemplate<T>& variableTemplate);

	std::unordered_map<std::string, VModuleVariable> m_variables;
	void* m_data;
};

class VModule {
  public:
	virtual ~VModule() = 0;


	virtual void OnCreate(VEngine& engine) = 0;
	virtual void OnDestroy(VEngine& engine) = 0;

	template <typename T> const T& getTypedOutput(const std::string_view& name) const;

  protected:
	template <typename T> void initializeTypedOutput(const std::string_view& name);

	template <typename T> void writeTypedOutput(const std::string_view& name, T& value);
  private:
	VModuleVariableContainer m_outputVariables;
};

template <typename T> inline const T& VModule::getTypedOutput(const std::string_view& name) const {
	auto iterator = m_outputVariables.find(name);
}

template <typename T> inline void VModule::initializeTypedOutput(const std::string_view& name) {}

template <typename T> inline void VModule::writeTypedOutput(const std::string_view& name, T& value) {}

template <typename... Ts>
inline VModuleVariableContainer::VModuleVariableContainer(const VModuleVariableTemplate<Ts> templates...) {
	m_data = std::malloc(sizeof(Ts) + ...);

	(allocateVariableTemplate(templates), ...);
}

template <typename T>
inline void VModuleVariableContainer::allocateVariableTemplate(const VModuleVariableTemplate<T>& variableTemplate) {

}
