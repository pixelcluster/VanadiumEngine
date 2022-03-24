#pragma once

// This header doesn't ship its includes, so that it can also be #include-d inside a namespace.
// This header needs:
// #include <vector>
// #include <variant>
// #define VK_NO_PROTOTYPES
// #include <vulkan/vulkan.h>

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

struct DescriptorBindingLayoutInfo {
	VkDescriptorSetLayoutBinding binding;
	bool usesImmutableSamplers;

	bool operator==(const DescriptorBindingLayoutInfo& other) const {
		return binding.binding == other.binding.binding && binding.descriptorCount == other.binding.descriptorCount &&
			   binding.descriptorType == other.binding.descriptorType &&
			   binding.stageFlags == other.binding.stageFlags && usesImmutableSamplers == other.usesImmutableSamplers;
	}
};

enum class PipelineType { Graphics, Compute };

struct CompiledShader {
	VkShaderStageFlagBits stage;
	void* data;
	size_t dataSize;
};