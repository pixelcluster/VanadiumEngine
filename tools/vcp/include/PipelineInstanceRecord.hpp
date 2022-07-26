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

#include <vector>
#include <fstream>
#include <Log.hpp>
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <spirv_reflect.h>

#include <json/json.h>
#include <VCPFormat.hpp>

#include <ParsingUtils.hpp>
#include <spirv_reflect.h>

#include <PipelineArchetypeRecord.hpp> //for ReflectedShader

class PipelineInstanceRecord {
  public:
  	PipelineInstanceRecord();
	PipelineInstanceRecord(PipelineType type, const std::string_view& srcPath, const Json::Value& instanceNode);

	void serialize(std::ofstream& outStream) const;

	void verifyInstance(const std::string_view& srcPath, const std::vector<ReflectedShader>& shaderModules);

	bool isValid() const { return m_isValid; }

  private:
	bool m_isValid = true;

	void deserializeVertexInput(const std::string_view& srcPath, const Json::Value& node);
	void deserializeInputAssembly(const std::string_view& srcPath, const Json::Value& node);
	void deserializeRasterization(const std::string_view& srcPath, const Json::Value& node);
	void deserializeViewportScissor(const std::string_view& srcPath, const Json::Value& node);
	void deserializeMultisample(const std::string_view& srcPath, const Json::Value& node);
	void deserializeDepthStencil(const std::string_view& srcPath, const Json::Value& node);
	VkStencilOpState deserializeStencilState(const std::string_view& srcPath, const Json::Value& node);
	void deserializeDynamicState(const std::string_view& srcPath, const Json::Value& node);
	void deserializeColorBlend(const std::string_view& srcPath, const Json::Value& node);
	void deserializeColorAttachmentBlend(const std::string_view& srcPath, const Json::Value& node);
	void deserializeSpecializationConfigs(const std::string_view& srcPath, const Json::Value& node);

	void verifyVertexShader(const std::string_view& srcPath, const SpvReflectShaderModule& shader);
	void verifyFragmentShader(const std::string_view& srcPath, const SpvReflectShaderModule& shader);
	
	PipelineType m_type;
	PipelineInstanceData m_data;
};

template<> inline void serialize(const PipelineInstanceRecord& record, std::ofstream& outStream) {
	record.serialize(outStream);
}
