#pragma once

#include <string_view>
#include <unordered_map>
#include <variant>
#include <gpu/VGPUResource.hpp>

class VEngine;

struct VCPUResource {
	void* data;
	size_t size;
};

enum class VInputOutputVariableType {
	Int,
	Float,
	Bool,
	String,
	CPUResource,
	GPUResource
};

struct VInputOutputVariable {
	VInputOutputVariableType type;
	std::variant<int, float, bool, std::string, VCPUResource, gpu::VGPUResource> data;
};

class VModule {
  public:
	virtual ~VModule() = 0;

	virtual void onCreate(VEngine& engine) = 0;
	virtual void onExecute(VEngine& engine) = 0;
	virtual void onDestroy(VEngine& engine) = 0;

	const VInputOutputVariable& outputVariable(const std::string_view& name) const;

  protected:
	void initializeTypedOutput(const std::string_view& name, VInputOutputVariableType type);

	void writeOutput(const std::string_view& name, int value);
	void writeOutput(const std::string_view& name, float value);
	void writeOutput(const std::string_view& name, bool value);
	void writeOutput(const std::string_view& name, const std::string& value);
	void writeOutput(const std::string_view& name, const VCPUResource& value);
	void writeOutput(const std::string_view& name, const gpu::VGPUResource& value);

  private:
	std::unordered_map<std::string, VInputOutputVariable> m_outputVariables;
};
