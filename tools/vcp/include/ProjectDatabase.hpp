#pragma once

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <vector>

#include <json/json.h>

struct InstanceVertexInputConfig {
	std::vector<VkVertexInputAttributeDescription> attributes;
	std::vector<VkVertexInputBindingDescription> bindings;
};

struct InstanceInputAssemblyConfig {
	VkPrimitiveTopology topology;
	bool primitiveRestart;
};

struct InstanceRasterizationConfig {
	bool depthClampEnable;
	bool rasterizerDiscardEnable;
	VkPolygonMode polygonMode;
	VkCullModeFlags cullMode;
	VkFrontFace frontFace;
	bool depthBiasEnable;
	float depthBiasConstantFactor;
	float depthBiasClamp;
	float depthBiasSlopeFactor;
	float lineWidth;
};

using InstanceMultisampleConfig = VkSampleCountFlags;

struct InstanceDepthStencilConfig {
	bool depthTestEnable;
	bool depthWriteEnable;
	VkCompareOp depthCompareOp;
	bool depthBoundsTestEnable;
	bool stencilTestEnable;
	VkStencilOpState front;
	VkStencilOpState back;
	float minDepthBounds;
	float maxDepthBounds;
};

struct InstanceColorBlendConfig {
	bool logicOpEnable;
	VkLogicOp logicOp;
	float blendConstants[4];
};

using InstanceColorAttachmentBlendConfig = VkPipelineColorBlendAttachmentState;

class ProjectDatabase {
  public:
	ProjectDatabase() {}

	void deserialize(const std::string_view& filename);

  private:
	void deserializeVertexInput(const Json::Value& node);
	void deserializeInputAssembly(const Json::Value& node);
	void deserializeRasterization(const Json::Value& node);
	void deserializeMultisample(const Json::Value& node);
	void deserializeDepthStencil(const Json::Value& node);
	VkStencilOpState deserializeStencilState(const Json::Value& node);
	void deserializeColorBlend(const Json::Value& node);
	void deserializeColorAttachmentBlend(const Json::Value& node);

	uint32_t asUIntOr(const Json::Value& value, const std::string_view& name, uint32_t fallback);
	float asFloatOr(const Json::Value& value, const std::string_view& name, float fallback);
	bool asBoolOr(const Json::Value& value, const std::string_view& name, bool fallback);
	const char* asCStringOr(const Json::Value& value, const std::string_view& name, const char* fallback);
	std::string asStringOr(const Json::Value& value, const std::string_view& name, const std::string& fallback);

	std::string m_name;

	std::vector<InstanceVertexInputConfig> m_instanceVertexInputConfigs;
	std::vector<InstanceInputAssemblyConfig> m_instanceInputAssemblyConfigs;
	std::vector<InstanceRasterizationConfig> m_instanceRasterizationConfigs;
	std::vector<InstanceMultisampleConfig> m_instanceMultisampleConfigs;
	std::vector<InstanceDepthStencilConfig> m_instanceDepthStencilConfigs;
	std::vector<InstanceColorBlendConfig> m_instanceColorBlendConfigs;
	std::vector<InstanceColorAttachmentBlendConfig> m_instanceColorAttachmentBlendConfigs;
};