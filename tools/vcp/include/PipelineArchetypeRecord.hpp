#pragma once

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <string_view>
#include <vector>

struct ShaderSourceFile {
	VkShaderStageFlagBits stage;
	std::string_view path;
};

struct VertexAttribute {
	uint32_t location;
	VkFormat format;
};

struct VertexInputConfig {
	std::vector<VertexAttribute> attributes;
};

enum class PipelineType {
	Graphics, Compute
};

class PipelineArchetypeRecord {
  public:
	void addShaderSource(ShaderSourceFile&& file);
  private:
	std::vector<ShaderSourceFile> m_files;

};