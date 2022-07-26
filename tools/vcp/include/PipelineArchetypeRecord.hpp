/* VanadiumEngine, a Vulkan rendering toolkit
 * Copyright (C) 2022 Friedrich Vock
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#pragma once

#include <string_view>
#include <fstream>
#include <Log.hpp>
#define VK_NO_PROTOTYPES
#include <spirv_reflect.h>
#include <vulkan/vulkan.h>

#include <VCPFormat.hpp>

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
	std::vector<ReflectedShader> retrieveCompileResults(const std::string_view& srcPath,
																   const std::string& tempDir);

	void verifyArchetype(const std::string_view& srcPath,
						 std::vector<std::vector<DescriptorBindingLayoutInfo>>& setLayoutInfos,
						 const std::vector<ReflectedShader>& shaders);

	void serialize(std::ofstream& outStream);

	bool isValid() const { return m_isValid; }

	PipelineType type() const { return m_pipelineType; }

	void freeShaders();

  private:
	bool m_isValid = false;

	PipelineType m_pipelineType;

	std::vector<ShaderSourceFile> m_files;
	std::vector<SubprocessID> m_compilerSubprocessIDs;
	std::vector<CompiledShader> m_compiledShaders;

	std::vector<uint32_t> m_setLayoutIndices;
	std::vector<VkPushConstantRange> m_pushConstantRanges;
};
