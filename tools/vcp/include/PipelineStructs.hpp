#pragma once

#include <string_view>
#include <variant>
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <spirv_reflect.h>

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

struct InstanceSpecializationConfig {
	VkSpecializationMapEntry mapEntry;
	std::variant<bool, uint32_t, float> data;
};

using InstanceColorAttachmentBlendConfig = VkPipelineColorBlendAttachmentState;

struct ShaderSourceFile {
	VkShaderStageFlagBits stage;
	std::string path;
};

struct CompiledShader {
	VkShaderStageFlagBits stage;
	void* data;
	size_t dataSize;
};

struct ReflectedShader {
	VkShaderStageFlagBits stage;
	SpvReflectShaderModule shader;
};

struct DescriptorSetLayoutInfo {
	uint32_t setIndex;
	VkDescriptorSetLayoutBinding binding;
	bool usesImmutableSamplers;
};