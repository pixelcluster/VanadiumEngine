#pragma once

#include <string_view>
#include <unordered_map>

class VEngine;

struct VModuleOutput {
	std::unordered_map<std::string, bool> booleans;
	std::unordered_map<std::string, float> floats;
	std::unordered_map<std::string, int> ints;
	std::unordered_map<std::string, VCPUResource> cpuResources;
	std::unordered_map<std::string, VGPUResourceInput> gpuResources;
	std::unordered_map<std::string, VGPUImageInput> gpuImages;
};

class VModule {
  public:
	virtual ~VModule() = 0;


	virtual void OnCreate(VEngine& engine) = 0;
	virtual void OnDestroy(VEngine& engine) = 0;

	template <typename T> const T& getTypedOutput(const std::string_view& name) const;

  protected:
	template <typename T> void writeTypedOutput(const std::string_view& name, T& value);
  private:
};