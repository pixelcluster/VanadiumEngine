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

	struct PipelineLibraryStageSpecialization
	{
		PipelineLibraryStageSpecialization() {}
		PipelineLibraryStageSpecialization(const PipelineLibraryStageSpecialization& other) = delete;
		PipelineLibraryStageSpecialization& operator=(const PipelineLibraryStageSpecialization& other) = delete;
		PipelineLibraryStageSpecialization(PipelineLibraryStageSpecialization&& other);
		PipelineLibraryStageSpecialization& operator=(PipelineLibraryStageSpecialization&& other) = delete;
		~PipelineLibraryStageSpecialization();

		VkShaderStageFlagBits stage;
		VkSpecializationInfo specializationInfo;
		std::vector<VkSpecializationMapEntry> mapEntries;
		char* specializationData;
	};
	

	struct PipelineLibraryGraphicsInstance {
		PipelineLibraryGraphicsInstance() {}
		PipelineLibraryGraphicsInstance(const PipelineLibraryGraphicsInstance& other) = delete;
		PipelineLibraryGraphicsInstance& operator=(const PipelineLibraryGraphicsInstance& other) = delete;
		PipelineLibraryGraphicsInstance(PipelineLibraryGraphicsInstance&& other);
		PipelineLibraryGraphicsInstance& operator=(PipelineLibraryGraphicsInstance&& other) = delete;

		uint32_t archetypeID;
		std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfos;
		std::vector<PipelineLibraryStageSpecialization> stageSpecializations;
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
		std::vector<VkDynamicState> dynamicStates;
		VkPipelineViewportStateCreateInfo viewportConfig;
		VkViewport viewport;
		VkRect2D scissorRect;
		VkGraphicsPipelineCreateInfo pipelineCreateInfo;
		VkPipelineLayout layout;
	};

	struct PipelineLibraryComputeInstance
	{
		uint32_t archetypeID;
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

		void createGraphicsPipeline(uint64_t& bufferOffset);
		void createComputePipeline(uint64_t& bufferOffset);

		std::vector<PipelineLibraryArchetype> m_archetypes;
		robin_hood::unordered_map<std::string, PipelineLibraryGraphicsInstance> m_graphicsInstanceNames;
		robin_hood::unordered_map<std::string, PipelineLibraryComputeInstance> m_computeInstanceNames;

		std::vector<VkDescriptorSetLayout> m_descriptorSetLayouts;
		std::vector<VkSampler> m_immutableSamplers;
	};

	template <typename T> T PipelineLibrary::readBuffer(uint64_t& currentOffset) {
		assertFatal(currentOffset + sizeof(T) <= m_fileSize, "PipelineLibrary: Invalid pipeline library file!\n");
		T value;
		std::memcpy(&value, reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(m_buffer) + currentOffset), sizeof(T));

		return value;
	}

} // namespace vanadium::graphics