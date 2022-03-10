#pragma once

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <string_view>
#include <vector>

#include <json/json.h>

struct ShaderSourceFile {
	VkShaderStageFlagBits stage;
	std::string_view path;
};

enum class PipelineType {
	Graphics, Compute
};

class PipelineArchetypeRecord {
  public:
	PipelineArchetypeRecord(const Json::Value& archetypeRoot);
  private:
	void addShaderSource(ShaderSourceFile&& file);

	std::vector<ShaderSourceFile> m_files;

	std::vector<uint32_t> m_setBindingOffsets;
	std::vector<VkDescriptorSetLayoutBinding> m_bindings;
};