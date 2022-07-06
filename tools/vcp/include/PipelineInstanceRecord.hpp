#pragma once 

#include <vector>
#include <variant>
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <spirv_reflect.h>

#include <json/json.h>
#include <PipelineStructs.hpp>

#include <ParsingUtils.hpp>
#include <spirv_reflect.h>

#include <PipelineArchetypeRecord.hpp> //for ReflectedShader

class PipelineInstanceRecord {
  public:
  PipelineInstanceRecord();
	PipelineInstanceRecord(PipelineType type, const std::string_view& srcPath, const Json::Value& instanceNode);

	size_t serializedSize() const;
	void serialize(void* data);

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

	void* serializeStencilState(const VkStencilOpState& state, void* data);

	void verifyVertexShader(const std::string_view& srcPath, const SpvReflectShaderModule& shader);
	void verifyFragmentShader(const std::string_view& srcPath, const SpvReflectShaderModule& shader);

	std::string m_name;
	PipelineType m_type;

	InstanceVertexInputConfig m_instanceVertexInputConfig;
	InstanceInputAssemblyConfig m_instanceInputAssemblyConfig;
	InstanceRasterizationConfig m_instanceRasterizationConfig;
	InstanceViewportScissorConfig m_instanceViewportScissorConfig;
	InstanceMultisampleConfig m_instanceMultisampleConfig;
	InstanceDepthStencilConfig m_instanceDepthStencilConfig;
	InstanceColorBlendConfig m_instanceColorBlendConfig;
	InstanceDynamicStateConfig m_instanceDynamicStateConfig;
	std::vector<InstanceColorAttachmentBlendConfig> m_instanceColorAttachmentBlendConfigs;

	std::vector<InstanceStageSpecializationConfig> m_instanceSpecializationConfigs;
};