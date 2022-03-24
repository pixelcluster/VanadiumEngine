#pragma once

#include <string_view>
#include <variant>
#define VK_NO_PROTOTYPES
#include <spirv_reflect.h>
#include <vulkan/vulkan.h>

#include <PipelineStructs.hpp>

#include <ParsingUtils.hpp>
#include <filesystem>
#include <string>
#include <vector>

#include <SysUtils.hpp>
#include <spirv_reflect.h>

#include <json/json.h>

struct ShaderSourceFile {
	VkShaderStageFlagBits stage;
	std::string path;
};

struct ReflectedShader {
	VkShaderStageFlagBits stage;
	SpvReflectShaderModule shader;
};

class PipelineArchetypeRecord {
  public:
	PipelineArchetypeRecord(const std::string_view& srcPath, const std::string& projectDir,
							std::vector<std::vector<DescriptorBindingLayoutInfo>>& setLayoutInfos,
							const Json::Value& archetypeRoot);

	void compileShaders(const std::string& tempDir, const std::string& compilerCommand,
						const std::vector<std::string>& additionalArgs);
	std::vector<ReflectedShader> retrieveCompileResults(const std::string_view& srcPath, const std::string& tempDir);

	void verifyArchetype(const std::string_view& srcPath,
						 std::vector<std::vector<DescriptorBindingLayoutInfo>>& setLayoutInfos,
						 const std::vector<ReflectedShader>& shaders);

	size_t serializedSize() const;
	void serialize(void* data);

	bool isValid() const { return m_isValid; }

	PipelineType type() const { return m_pipelineType; }

  private:
	bool m_isValid = false;

	PipelineType m_pipelineType;

	std::vector<ShaderSourceFile> m_files;
	std::vector<SubprocessID> m_compilerSubprocessIDs;
	std::vector<CompiledShader> m_compiledShaders;

	std::vector<uint32_t> m_setLayoutIndices;
	std::vector<VkPushConstantRange> m_pushConstantRanges;
};