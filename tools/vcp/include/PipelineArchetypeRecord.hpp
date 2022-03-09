#pragma once

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <string_view>
#include <vector>

struct ShaderSourceFile {
	VkShaderStageFlagBits stage;
	std::string_view path;
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