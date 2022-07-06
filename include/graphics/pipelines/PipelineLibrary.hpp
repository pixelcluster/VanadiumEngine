#pragma once

#include <Log.hpp>
#include <graphics/DeviceContext.hpp>
#include <variant>
#include <util/Vector.hpp>
#include <graphics/RenderPassSignature.hpp>

namespace vanadium::graphics {

#include <tools/vcp/include/PipelineStructs.hpp>

	constexpr uint32_t pipelineFileVersion = 5;

	struct DescriptorSetLayoutInfo {
		VkDescriptorSetLayout layout;
		SimpleVector<VkDescriptorSetLayoutBinding> bindingInfos;
	};

	struct PipelineLibraryArchetype {
		PipelineType type;
		SimpleVector<VkShaderModule> shaderModules;
		SimpleVector<uint32_t> setLayoutIndices;
		SimpleVector<VkPushConstantRange> pushConstantRanges;
	};

	struct PipelineLibraryStageSpecialization {
		PipelineLibraryStageSpecialization() {}
		PipelineLibraryStageSpecialization(const PipelineLibraryStageSpecialization& other) = delete;
		PipelineLibraryStageSpecialization& operator=(const PipelineLibraryStageSpecialization& other) = delete;
		PipelineLibraryStageSpecialization(PipelineLibraryStageSpecialization&& other) = default;
		PipelineLibraryStageSpecialization& operator=(PipelineLibraryStageSpecialization&& other) = delete;
		~PipelineLibraryStageSpecialization() {
			delete[] specializationData;
		}

		VkShaderStageFlagBits stage;
		VkSpecializationInfo specializationInfo;
		SimpleVector<VkSpecializationMapEntry> mapEntries;
		char* specializationData;
	};


	struct PipelineLibraryGraphicsInstance {
		PipelineLibraryGraphicsInstance() {}
		PipelineLibraryGraphicsInstance(const PipelineLibraryGraphicsInstance& other) = delete;
		PipelineLibraryGraphicsInstance& operator=(const PipelineLibraryGraphicsInstance& other) = delete;
		PipelineLibraryGraphicsInstance(PipelineLibraryGraphicsInstance&& other);
		PipelineLibraryGraphicsInstance& operator=(PipelineLibraryGraphicsInstance&& other) = delete;

		uint32_t archetypeID;
		std::string name;
		SimpleVector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfos;
		SimpleVector<PipelineLibraryStageSpecialization> stageSpecializations;
		SimpleVector<VkVertexInputAttributeDescription> attribDescriptions;
		SimpleVector<VkVertexInputBindingDescription> bindingDescriptions;
		VkPipelineVertexInputStateCreateInfo vertexInputConfig;
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyConfig;
		VkPipelineRasterizationStateCreateInfo rasterizationConfig;
		VkPipelineMultisampleStateCreateInfo multisampleConfig;
		VkPipelineDepthStencilStateCreateInfo depthStencilConfig;
		VkPipelineColorBlendStateCreateInfo colorBlendConfig;
		SimpleVector<VkPipelineColorBlendAttachmentState> colorAttachmentBlendConfigs;
		VkPipelineDynamicStateCreateInfo dynamicStateConfig;
		SimpleVector<VkDynamicState> dynamicStates;
		VkPipelineViewportStateCreateInfo viewportConfig;
		SimpleVector<VkViewport> viewports;
		SimpleVector<VkRect2D> scissorRects;
		VkGraphicsPipelineCreateInfo pipelineCreateInfo;

		robin_hood::unordered_map<RenderPassSignature, VkPipeline> pipelines;
	};

	struct PipelineLibraryComputeInstance {
		uint32_t archetypeID;
		std::string name;
		VkPipeline pipeline;
		VkPipelineLayout layout;
	};

	class PipelineLibrary {
	  public:
		PipelineLibrary() {}

		void create(const std::string_view& libraryFileName, DeviceContext* deviceContext);

		void createForPass(const RenderPassSignature& signature, VkRenderPass pass,
						   const SimpleVector<uint32_t>& pipelineIDs);

		// these methods are essentially const but the user can modify state using the pipeline handles
		VkPipeline graphicsPipeline(uint32_t id, const RenderPassSignature& signature) {
			return m_graphicsInstances[id].pipelines[signature];
		}
		VkPipeline computePipeline(uint32_t id) { return m_computeInstances[id].pipeline; }

		const DescriptorSetLayoutInfo& graphicsPipelineSet(uint32_t id, uint32_t setIndex) {
			return m_descriptorSetLayouts[m_archetypes[m_graphicsInstances[id].archetypeID].setLayoutIndices[setIndex]];
		}
		SimpleVector<DescriptorSetLayoutInfo> graphicsPipelineSets(uint32_t id);
		VkPipelineLayout graphicsPipelineLayout(uint32_t id) {
			return m_graphicsInstances[id].pipelineCreateInfo.layout;
		}
		const SimpleVector<VkPushConstantRange>& graphicsPipelinePushConstantRanges(uint32_t id) const {
			return m_archetypes[m_graphicsInstances[id].archetypeID].pushConstantRanges;
		}
		const DescriptorSetLayoutInfo& computePipelineSet(uint32_t id, uint32_t setIndex) {
			return m_descriptorSetLayouts[m_archetypes[m_computeInstances[id].archetypeID].setLayoutIndices[setIndex]];
		}
		SimpleVector<DescriptorSetLayoutInfo> computePipelineSets(uint32_t id);
		VkPipelineLayout computePipelineLayout(uint32_t id) {
			return m_computeInstances[id].layout;
		}
		const SimpleVector<VkPushConstantRange>& computePipelinePushConstantRanges(uint32_t id) const {
			return m_archetypes[m_computeInstances[id].archetypeID].pushConstantRanges;
		}

		std::string_view graphicsPipelineName(uint32_t id) const { return m_graphicsInstances[id].name; }
		std::string_view computePipelineName(uint32_t id) const { return m_computeInstances[id].name; }

		uint32_t findGraphicsPipeline(const std::string_view& name);
		uint32_t findComputePipeline(const std::string_view& name);

		void destroy();

	  private:
		DeviceContext* m_deviceContext;
		void* m_buffer;
		size_t m_fileSize;

		template <typename T> T readBuffer(uint64_t& currentOffset);

		VkStencilOpState readStencilState(uint64_t& offset);

		void createGraphicsPipeline(uint64_t& bufferOffset);
		void createComputePipeline(uint64_t& bufferOffset);

		SimpleVector<PipelineLibraryArchetype> m_archetypes;
		SimpleVector<PipelineLibraryGraphicsInstance> m_graphicsInstances;
		SimpleVector<PipelineLibraryComputeInstance> m_computeInstances;

		SimpleVector<DescriptorSetLayoutInfo> m_descriptorSetLayouts;
		SimpleVector<VkSampler> m_immutableSamplers;
	};

	template <typename T> T PipelineLibrary::readBuffer(uint64_t& currentOffset) {
		assertFatal(currentOffset + sizeof(T) <= m_fileSize, "PipelineLibrary: Invalid pipeline library file!");
		T value;
		std::memcpy(&value, reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(m_buffer) + currentOffset), sizeof(T));
		currentOffset += sizeof(T);
		return value;
	}

} // namespace vanadium::graphics