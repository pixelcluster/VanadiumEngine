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

struct DescriptorSetLayoutInfo {
	VkDescriptorSetLayoutBinding binding;
	bool usesImmutableSamplers;
};

class PipelineArchetypeRecord {
  public:
	PipelineArchetypeRecord(const Json::Value& archetypeRoot);
  private:
	PipelineType m_pipelineType;

	std::vector<ShaderSourceFile> m_files;

	std::vector<uint32_t> m_setBindingOffsets;
	std::vector<DescriptorSetLayoutInfo> m_bindings;
};