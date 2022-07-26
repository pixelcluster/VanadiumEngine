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

#include <Log.hpp>
#include <graphics/DeviceContext.hpp>
#include <fstream>
#include <vector>
#include <graphics/RenderPassSignature.hpp>

namespace vanadium::graphics {

#include <tools/vcp/include/VCPFormat.hpp>

	constexpr uint32_t pipelineFileVersion = 5;

	struct DescriptorSetLayoutInfo {
		VkDescriptorSetLayout layout;
		std::vector<VkDescriptorSetLayoutBinding> bindingInfos;
	};

	struct PipelineLibraryArchetype {
		PipelineType type;
		std::vector<VkShaderModule> shaderModules;
		std::vector<uint32_t> setLayoutIndices;
		std::vector<VkPushConstantRange> pushConstantRanges;
	};

	struct PipelineLibraryStageSpecialization {
		PipelineLibraryStageSpecialization() {}
		PipelineLibraryStageSpecialization(const PipelineLibraryStageSpecialization& other) = delete;
		PipelineLibraryStageSpecialization& operator=(const PipelineLibraryStageSpecialization& other) = delete;
		PipelineLibraryStageSpecialization(PipelineLibraryStageSpecialization&& other) = default;
		PipelineLibraryStageSpecialization& operator=(PipelineLibraryStageSpecialization&& other) = delete;
		~PipelineLibraryStageSpecialization() {
			if(specializationData)
				delete[] specializationData;
		}

		VkShaderStageFlagBits stage;
		VkSpecializationInfo specializationInfo;
		std::vector<VkSpecializationMapEntry> mapEntries;
		char* specializationData = nullptr;
	};


	struct PipelineLibraryGraphicsInstance {
		PipelineLibraryGraphicsInstance() {}
		PipelineLibraryGraphicsInstance(const PipelineLibraryGraphicsInstance& other) = delete;
		PipelineLibraryGraphicsInstance& operator=(const PipelineLibraryGraphicsInstance& other) = delete;
		PipelineLibraryGraphicsInstance(PipelineLibraryGraphicsInstance&& other);
		PipelineLibraryGraphicsInstance& operator=(PipelineLibraryGraphicsInstance&& other) = delete;

		uint32_t archetypeID;
		std::string name;
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
		std::vector<VkViewport> viewports;
		std::vector<VkRect2D> scissorRects;
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
						   const std::vector<uint32_t>& pipelineIDs);

		// these methods are essentially const but the user can modify state using the pipeline handles
		VkPipeline graphicsPipeline(uint32_t id, const RenderPassSignature& signature) {
			return m_graphicsInstances[id].pipelines[signature];
		}
		VkPipeline computePipeline(uint32_t id) { return m_computeInstances[id].pipeline; }

		const DescriptorSetLayoutInfo& graphicsPipelineSet(uint32_t id, uint32_t setIndex) {
			return m_descriptorSetLayouts[m_archetypes[m_graphicsInstances[id].archetypeID].setLayoutIndices[setIndex]];
		}
		std::vector<DescriptorSetLayoutInfo> graphicsPipelineSets(uint32_t id);
		VkPipelineLayout graphicsPipelineLayout(uint32_t id) {
			return m_graphicsInstances[id].pipelineCreateInfo.layout;
		}
		const std::vector<VkPushConstantRange>& graphicsPipelinePushConstantRanges(uint32_t id) const {
			return m_archetypes[m_graphicsInstances[id].archetypeID].pushConstantRanges;
		}
		const DescriptorSetLayoutInfo& computePipelineSet(uint32_t id, uint32_t setIndex) {
			return m_descriptorSetLayouts[m_archetypes[m_computeInstances[id].archetypeID].setLayoutIndices[setIndex]];
		}
		std::vector<DescriptorSetLayoutInfo> computePipelineSets(uint32_t id);
		VkPipelineLayout computePipelineLayout(uint32_t id) {
			return m_computeInstances[id].layout;
		}
		const std::vector<VkPushConstantRange>& computePipelinePushConstantRanges(uint32_t id) const {
			return m_archetypes[m_computeInstances[id].archetypeID].pushConstantRanges;
		}

		std::string_view graphicsPipelineName(uint32_t id) const { return m_graphicsInstances[id].name; }
		std::string_view computePipelineName(uint32_t id) const { return m_computeInstances[id].name; }

		uint32_t findGraphicsPipeline(const std::string_view& name);
		uint32_t findComputePipeline(const std::string_view& name);

		void destroy();

	  private:
		DeviceContext* m_deviceContext;
		std::ifstream m_fileStream;

		void createGraphicsPipeline();
		void createComputePipeline();

		std::vector<PipelineLibraryArchetype> m_archetypes;
		std::vector<PipelineLibraryGraphicsInstance> m_graphicsInstances;
		std::vector<PipelineLibraryComputeInstance> m_computeInstances;

		std::vector<DescriptorSetLayoutInfo> m_descriptorSetLayouts;
		std::vector<VkSampler> m_immutableSamplers;
	};

} // namespace vanadium::graphics
