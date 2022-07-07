#pragma once

// This header doesn't include its dependencies itself, so that it can also be #include-d inside a namespace.
// This header needs:
// #include <vector>
// #include <fstream>
// #define VK_NO_PROTOTYPES
// #include <vulkan/vulkan.h>

#ifdef VCP_EXPORTER_DIAGNOSTICS
#include <typeinfo>
#endif

template <typename T> inline void serialize(const T& data, std::ofstream& outStream) {
#ifdef VCP_EXPORTER_DIAGNOSTICS
	vanadium::logInfo("Serializing {} at offset {} (size = {})", std::string(typeid(T).name()), outStream.tellp(),
					  sizeof(T));
#endif
	outStream.write(reinterpret_cast<const char*>(&data), sizeof(data));
}
template <typename T> inline T deserialize(std::ifstream& inStream) {
	vanadium::assertFatal(inStream.good(), "Invalid VCP file!\n");
	T t;
	char data[sizeof(T)];
	inStream.read(data, sizeof(T));
	std::memcpy(&t, data, sizeof(T));
	return t;
}

template <> inline void serialize(const std::string& data, std::ofstream& outStream) {
	uint32_t count = static_cast<uint32_t>(data.size());
#ifdef VCP_EXPORTER_DIAGNOSTICS
	vanadium::logInfo("Serializing std::string at offset {} (size = {})", outStream.tellp(), sizeof(uint32_t) + count);
#endif
	outStream.write(reinterpret_cast<char*>(&count), sizeof(uint32_t));
	outStream.write(data.data(), count);
}
template <> inline std::string deserialize(std::ifstream& inStream) {
	vanadium::assertFatal(inStream.good(), "Invalid VCP file!\n");
	uint32_t count;
	inStream.read(reinterpret_cast<char*>(&count), sizeof(uint32_t));
	std::string result = std::string(count, '?');
	inStream.read(result.data(), count);
	return result;
}

template <typename T> inline void serializeVector(const std::vector<T>& data, std::ofstream& outStream) {
	uint32_t count = static_cast<uint32_t>(data.size());
#ifdef VCP_EXPORTER_DIAGNOSTICS
	vanadium::logInfo("Serializing std::vector<{}> at offset {} (size = {})", std::string(typeid(T).name()), outStream.tellp(),
					  sizeof(uint32_t) + count);
#endif
	outStream.write(reinterpret_cast<char*>(&count), sizeof(uint32_t));
	for (auto& element : data) {
		serialize(element, outStream);
	}
}

template <typename T> inline std::vector<T> deserializeVector(std::ifstream& inStream) {
	vanadium::assertFatal(inStream.good(), "Invalid VCP file!\n");
	uint32_t count;
	inStream.read(reinterpret_cast<char*>(&count), sizeof(uint32_t));

	std::vector<T> result;
	result.reserve(count);
	for (uint32_t i = 0; i < count; ++i) {
		result.push_back(deserialize<T>(inStream));
	}
	return result;
}

constexpr uint32_t vcpFileVersion = 5;
constexpr uint32_t vcpMagicNumber = 0x115CDBEF;

struct VCPFileHeader {
	uint32_t magic = vcpMagicNumber;
	uint32_t version = vcpFileVersion;
};

struct PipelineInstanceVertexInputConfig {
	std::vector<VkVertexInputAttributeDescription> attributes;
	std::vector<VkVertexInputBindingDescription> bindings;
};

template <>
inline void serialize<PipelineInstanceVertexInputConfig>(const PipelineInstanceVertexInputConfig& data,
														 std::ofstream& outStream) {
	serializeVector(data.attributes, outStream);
	serializeVector(data.bindings, outStream);
}

template <>
inline PipelineInstanceVertexInputConfig deserialize<PipelineInstanceVertexInputConfig>(std::ifstream& inStream) {
	return { .attributes = deserializeVector<VkVertexInputAttributeDescription>(inStream),
			 .bindings = deserializeVector<VkVertexInputBindingDescription>(inStream) };
}

struct PipelineInstanceInputAssemblyConfig {
	VkPrimitiveTopology topology;
	bool primitiveRestart;
};

struct PipelineInstanceRasterizationConfig {
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

using PipelineInstanceMultisampleConfig = VkSampleCountFlagBits;

struct PipelineInstanceViewportScissorConfig {
	std::vector<VkViewport> viewports;
	std::vector<VkRect2D> scissorRects;
};

template <>
inline void serialize<PipelineInstanceViewportScissorConfig>(const PipelineInstanceViewportScissorConfig& data,
															 std::ofstream& outStream) {
	serializeVector(data.viewports, outStream);
	serializeVector(data.scissorRects, outStream);
}

template <>
inline PipelineInstanceViewportScissorConfig deserialize<PipelineInstanceViewportScissorConfig>(
	std::ifstream& inStream) {
	return { .viewports = deserializeVector<VkViewport>(inStream),
			 .scissorRects = deserializeVector<VkRect2D>(inStream) };
}

struct PipelineInstanceDepthStencilConfig {
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

struct PipelineInstanceDynamicStateConfig {
	std::vector<VkDynamicState> dynamicStates;
};

template <>
inline void serialize<PipelineInstanceDynamicStateConfig>(const PipelineInstanceDynamicStateConfig& data,
														  std::ofstream& outStream) {
	serializeVector(data.dynamicStates, outStream);
}

template <>
inline PipelineInstanceDynamicStateConfig deserialize<PipelineInstanceDynamicStateConfig>(std::ifstream& inStream) {
	return { .dynamicStates = deserializeVector<VkDynamicState>(inStream) };
}

struct PipelineInstanceColorBlendConfig {
	bool logicOpEnable;
	VkLogicOp logicOp;
	float blendConstants[4];
};

enum class SpecializationDataType { Boolean, UInt32, Float };

struct PipelineInstanceSpecializationConfig {
	VkSpecializationMapEntry mapEntry;
	SpecializationDataType type;
	union {
		bool dataBool;
		uint32_t dataUint32;
		float dataFloat;
	};
};

struct PipelineInstanceStageSpecializationConfig {
	VkShaderStageFlagBits stage;
	size_t specializationDataSize;
	std::vector<PipelineInstanceSpecializationConfig> configs;
};

template <>
inline void serialize<PipelineInstanceStageSpecializationConfig>(const PipelineInstanceStageSpecializationConfig& data,
																 std::ofstream& outStream) {
	serialize(data.stage, outStream);
	serialize(data.specializationDataSize, outStream);
	serializeVector(data.configs, outStream);
}

template <>
inline PipelineInstanceStageSpecializationConfig deserialize<PipelineInstanceStageSpecializationConfig>(
	std::ifstream& inStream) {
	return { .stage = deserialize<VkShaderStageFlagBits>(inStream),
			 .specializationDataSize = deserialize<size_t>(inStream),
			 .configs = deserializeVector<PipelineInstanceSpecializationConfig>(inStream) };
}

using PipelineInstanceColorAttachmentBlendConfig = VkPipelineColorBlendAttachmentState;

struct SamplerInfo {
	VkFilter magFilter;
	VkFilter minFilter;
	VkSamplerMipmapMode mipmapMode;
	VkSamplerAddressMode addressModeU;
	VkSamplerAddressMode addressModeV;
	VkSamplerAddressMode addressModeW;
	float mipLodBias;
	bool anisotropyEnable;
	float maxAnisotropy;
	bool compareEnable;
	VkCompareOp compareOp;
	float minLod;
	float maxLod;
	VkBorderColor borderColor;
	bool unnormalizedCoordinates;

	bool operator==(const SamplerInfo& other) const {
		return magFilter == other.magFilter && minFilter == other.minFilter && mipmapMode == other.mipmapMode &&
			   addressModeU == other.addressModeU && addressModeV == other.addressModeV &&
			   addressModeW == other.addressModeW && mipLodBias == other.mipLodBias &&
			   anisotropyEnable == other.anisotropyEnable && maxAnisotropy == other.maxAnisotropy &&
			   compareEnable == other.compareEnable && compareOp == other.compareOp && minLod == other.minLod &&
			   maxLod == other.maxLod && borderColor == other.borderColor &&
			   unnormalizedCoordinates == other.unnormalizedCoordinates;
	}
};

struct DescriptorBindingLayoutInfo {
	VkDescriptorSetLayoutBinding binding;
	bool usesImmutableSamplers;
	std::vector<SamplerInfo> immutableSamplerInfos;

	bool operator==(const DescriptorBindingLayoutInfo& other) const {
		return binding.binding == other.binding.binding && binding.descriptorCount == other.binding.descriptorCount &&
			   binding.descriptorType == other.binding.descriptorType &&
			   binding.stageFlags == other.binding.stageFlags && usesImmutableSamplers == other.usesImmutableSamplers &&
			   immutableSamplerInfos == other.immutableSamplerInfos;
	}
};

template <>
inline void serializeVector(const std::vector<std::vector<DescriptorBindingLayoutInfo>>& data,
							std::ofstream& outStream) {
	uint32_t count = static_cast<uint32_t>(data.size());
	outStream.write(reinterpret_cast<char*>(&count), sizeof(uint32_t));
	for (auto& element : data) {
		serializeVector(element, outStream);
	}
}

template <> inline std::vector<std::vector<DescriptorBindingLayoutInfo>> deserializeVector(std::ifstream& inStream) {
	uint32_t count;
	inStream.read(reinterpret_cast<char*>(&count), sizeof(uint32_t));

	std::vector<std::vector<DescriptorBindingLayoutInfo>> result;
	result.reserve(count);
	for (uint32_t i = 0; i < count; ++i) {
		result.push_back(deserializeVector<DescriptorBindingLayoutInfo>(inStream));
	}
	return result;
}

template <>
inline void serialize<DescriptorBindingLayoutInfo>(const DescriptorBindingLayoutInfo& data, std::ofstream& outStream) {
	serialize(data.binding, outStream);
	serialize(data.usesImmutableSamplers, outStream);
	serializeVector(data.immutableSamplerInfos, outStream);
}

template <> inline DescriptorBindingLayoutInfo deserialize<DescriptorBindingLayoutInfo>(std::ifstream& inStream) {
	return { .binding = deserialize<VkDescriptorSetLayoutBinding>(inStream),
			 .usesImmutableSamplers = deserialize<bool>(inStream),
			 .immutableSamplerInfos = deserializeVector<SamplerInfo>(inStream) };
}

enum class PipelineType { Graphics, Compute };

struct CompiledShader {
	VkShaderStageFlagBits stage;
	size_t dataSize;
	void* data;
};

template <> inline void serialize<CompiledShader>(const CompiledShader& data, std::ofstream& outStream) {
#ifdef VCP_EXPORTER_DIAGNOSTICS
	vanadium::logInfo("Serializing CompiledShader at offset {} (size = {})", outStream.tellp(),
					  sizeof(VkShaderStageFlagBits) + sizeof(size_t) + data.dataSize);
#endif
	serialize(data.stage, outStream);
	serialize(data.dataSize, outStream);
	outStream.write(static_cast<char*>(data.data), data.dataSize);
}

template <> inline CompiledShader deserialize<CompiledShader>(std::ifstream& inStream) {
	CompiledShader result = { .stage = deserialize<VkShaderStageFlagBits>(inStream),
							  .dataSize = deserialize<size_t>(inStream) };
	result.data = new char[result.dataSize];
	inStream.read(static_cast<char*>(result.data), result.dataSize);
	return result;
}

struct PipelineInstanceData {
	std::string name;
	PipelineInstanceVertexInputConfig instanceVertexInputConfig;
	PipelineInstanceInputAssemblyConfig instanceInputAssemblyConfig;
	PipelineInstanceRasterizationConfig instanceRasterizationConfig;
	PipelineInstanceViewportScissorConfig instanceViewportScissorConfig;
	PipelineInstanceMultisampleConfig instanceMultisampleConfig;
	PipelineInstanceDepthStencilConfig instanceDepthStencilConfig;
	PipelineInstanceColorBlendConfig instanceColorBlendConfig;
	PipelineInstanceDynamicStateConfig instanceDynamicStateConfig;
	std::vector<PipelineInstanceColorAttachmentBlendConfig> instanceColorAttachmentBlendConfigs;
	std::vector<PipelineInstanceStageSpecializationConfig> instanceSpecializationConfigs;
};

template <> inline void serialize<PipelineInstanceData>(const PipelineInstanceData& data, std::ofstream& outStream) {
#ifdef VCP_EXPORTER_DIAGNOSTICS
	vanadium::logInfo("Serializing PipelineInstanceData at offset {} (size = {})", outStream.tellp(),
					  sizeof(PipelineInstanceData));
#endif
	serialize(data.name, outStream);
	serialize(data.instanceVertexInputConfig, outStream);
	serialize(data.instanceInputAssemblyConfig, outStream);
	serialize(data.instanceRasterizationConfig, outStream);
	serialize(data.instanceViewportScissorConfig, outStream);
	serialize(data.instanceMultisampleConfig, outStream);
	serialize(data.instanceDepthStencilConfig, outStream);
	serialize(data.instanceColorBlendConfig, outStream);
	serialize(data.instanceDynamicStateConfig, outStream);
	serializeVector(data.instanceColorAttachmentBlendConfigs, outStream);
	serializeVector(data.instanceSpecializationConfigs, outStream);
}

template <> inline PipelineInstanceData deserialize<PipelineInstanceData>(std::ifstream& inStream) {
	return { .name = deserialize<std::string>(inStream),
			 .instanceVertexInputConfig = deserialize<PipelineInstanceVertexInputConfig>(inStream),
			 .instanceInputAssemblyConfig = deserialize<PipelineInstanceInputAssemblyConfig>(inStream),
			 .instanceRasterizationConfig = deserialize<PipelineInstanceRasterizationConfig>(inStream),
			 .instanceViewportScissorConfig = deserialize<PipelineInstanceViewportScissorConfig>(inStream),
			 .instanceMultisampleConfig = deserialize<PipelineInstanceMultisampleConfig>(inStream),
			 .instanceDepthStencilConfig = deserialize<PipelineInstanceDepthStencilConfig>(inStream),
			 .instanceColorBlendConfig = deserialize<PipelineInstanceColorBlendConfig>(inStream),
			 .instanceDynamicStateConfig = deserialize<PipelineInstanceDynamicStateConfig>(inStream),
			 .instanceColorAttachmentBlendConfigs =
				 deserializeVector<PipelineInstanceColorAttachmentBlendConfig>(inStream),
			 .instanceSpecializationConfigs = deserializeVector<PipelineInstanceStageSpecializationConfig>(inStream) };
}
