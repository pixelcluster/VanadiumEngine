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
#include <util/Vector.hpp>

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
							vanadium::SimpleVector<vanadium::SimpleVector<DescriptorBindingLayoutInfo>>& setLayoutInfos,
							const Json::Value& archetypeRoot);

	void compileShaders(const std::string& tempDir, const std::string& compilerCommand,
						const vanadium::SimpleVector<std::string>& additionalArgs);
	vanadium::SimpleVector<ReflectedShader> retrieveCompileResults(const std::string_view& srcPath,
																   const std::string& tempDir);

	void verifyArchetype(const std::string_view& srcPath,
						 vanadium::SimpleVector<vanadium::SimpleVector<DescriptorBindingLayoutInfo>>& setLayoutInfos,
						 const vanadium::SimpleVector<ReflectedShader>& shaders);

	size_t serializedSize() const;
	void serialize(void* data);

	bool isValid() const { return m_isValid; }

	PipelineType type() const { return m_pipelineType; }

	void freeShaders();

  private:
	bool m_isValid = false;

	PipelineType m_pipelineType;

	vanadium::SimpleVector<ShaderSourceFile> m_files;
	vanadium::SimpleVector<SubprocessID> m_compilerSubprocessIDs;
	vanadium::SimpleVector<CompiledShader> m_compiledShaders;

	vanadium::SimpleVector<uint32_t> m_setLayoutIndices;
	vanadium::SimpleVector<VkPushConstantRange> m_pushConstantRanges;
};