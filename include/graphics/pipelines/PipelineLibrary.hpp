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
		std::vector<uint32_t> m_setLayoutIndices;
		std::vector<VkPushConstantRange> m_pushConstantRanges;
	};

	struct PipelineLibraryInstance {
		PipelineLibraryInstance() {}
		PipelineLibraryInstance(const PipelineLibraryInstance& other) = delete;
		PipelineLibraryInstance(PipelineLibraryInstance&& other) = delete;

		uint32_t archetypeID;
		std::vector<VkVertexInputAttributeDescription> attribDescriptions;
		std::vector<VkVertexInputBindingDescription> bindingDescriptions;
		VkPipelineVertexInputStateCreateInfo vertexInputConfig;
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyConfig;
		VkPipelineRasterizationStateCreateInfo rasterizationConfig;
		VkPipelineMultisampleStateCreateInfo multisampleConfig;
		VkStencilOpState frontStencilState;
		VkPipelineDepthStencilStateCreateInfo depthStencilConfig;
		VkPipelineColorBlendStateCreateInfo colorBlendConfig;
		std::vector<VkPipelineColorBlendAttachmentState> colorAttachmentBlendConfigs;
		VkSpecializationInfo specializationInfo;
		std::vector<VkSpecializationMapEntry> mapEntries;
		char* specializationData;
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
		robin_hood::unordered_map<std::string, PipelineLibraryInstance> m_instanceNames;

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