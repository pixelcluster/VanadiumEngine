#pragma once

#include <Log.hpp>
#include <graphics/DeviceContext.hpp>
#include <variant>
#include <vector>

namespace vanadium::graphics {

#include <tools/vcp/include/PipelineStructs.hpp>

	constexpr uint32_t pipelineFileVersion = 4;

	struct DescriptorSetLayoutInfo {
		VkDescriptorSetLayout layout;
		std::vector<DescriptorBindingLayoutInfo> bindingInfos;
	};

	struct PipelineLibraryArchetype {
		PipelineType type;
		std::vector<VkShaderModule> shaderModules;
		std::vector<uint32_t> m_setLayoutIndices;
		std::vector<VkPushConstantRange> m_pushConstantRanges;
	};

	struct PipelineLibraryGraphicsInstance {
		PipelineLibraryGraphicsInstance() {}
		PipelineLibraryGraphicsInstance(const PipelineLibraryGraphicsInstance& other) = delete;
		PipelineLibraryGraphicsInstance(PipelineLibraryGraphicsInstance&& other);

		uint32_t archetypeID;
		std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfos;
		std::vector<VkSpecializationInfo> specializationInfos;
		std::vector<VkVertexInputAttributeDescription> attribDescriptions;
		std::vector<VkVertexInputBindingDescription> bindingDescriptions;
		VkPipelineVertexInputStateCreateInfo vertexInputConfig;
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyConfig;
		VkPipelineRasterizationStateCreateInfo rasterizationConfig;
		VkPipelineMultisampleStateCreateInfo multisampleConfig;
		VkPipelineDepthStencilStateCreateInfo depthStencilConfig;
		VkPipelineColorBlendStateCreateInfo colorBlendConfig;
		std::vector<VkPipelineColorBlendAttachmentState> colorAttachmentBlendConfigs;
		VkPipelineDynamicStateCreateInfo dynamicStateConfig;
		VkPipelineViewportStateCreateInfo viewportConfig;
		std::vector<VkViewport> viewports;
		std::vector<VkRect2D> scissorRects;
		VkSpecializationInfo specializationInfo;
		std::vector<VkSpecializationMapEntry> mapEntries;
		char* specializationData;
		VkGraphicsPipelineCreateInfo pipelineCreateInfo;
		VkPipelineLayout layout;
	};

	struct PipelineLibraryComputeInstance
	{
		PipelineLibraryComputeInstance() {}
		PipelineLibraryComputeInstance(const PipelineLibraryComputeInstance& other) = delete;
		PipelineLibraryComputeInstance(PipelineLibraryComputeInstance&& other);

		uint32_t archetypeID;
		std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfos;
		std::vector<VkSpecializationInfo> specializationInfos;
		std::vector<VkSpecializationMapEntry> mapEntries;
		char* specializationData;
		VkComputePipelineCreateInfo createInfo;
		VkPipeline pipeline;
		VkPipelineLayout layout;
	};
	

	class PipelineLibrary {
	  public:
		PipelineLibrary() {}

		void create(const std::string& libraryFileName, DeviceContext* deviceContext);

	  private:
		DeviceContext* m_deviceContext;
		void* m_buffer;
		size_t m_fileSize;

		template <typename T> T readBuffer(uint64_t& currentOffset);

		VkStencilOpState readStencilState(uint64_t& offset);

		void createGraphicsPipeline(uint64_t& bufferOffset, std::mutex& pipelineWriteMutex);
		void createComputePipeline(uint64_t& bufferOffset, std::mutex& pipelineWriteMutex);

		std::vector<PipelineLibraryArchetype> m_archetypes;
		robin_hood::unordered_map<std::string, PipelineLibraryGraphicsInstance> m_graphicsInstanceNames;
		robin_hood::unordered_map<std::string, PipelineLibraryComputeInstance> m_computeInstanceNames;

		std::vector<VkDescriptorSetLayout> m_descriptorSetLayouts;
		std::vector<VkSampler> m_immutableSamplers;
	};

	template <typename T> T PipelineLibrary::readBuffer(uint64_t& currentOffset) {
		assertFatal(currentOffset + sizeof(T) <= fileSize, "PipelineLibrary: Invalid pipeline library file!\n");
		T value;
		std::memcpy(&value, reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(buffer) + currentOffset), sizeof(T));

		return value;
	}

} // namespace vanadium::graphics