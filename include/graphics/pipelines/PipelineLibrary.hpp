#pragma once

#include <variant>
#include <vector>
#include <graphics/DeviceContext.hpp>

namespace vanadium::graphics {

#include <tools/vcp/include/PipelineStructs.hpp>

	struct DescriptorSetLayoutInfo {
		VkDescriptorSetLayout layout;
		std::vector<DescriptorBindingLayoutInfo> bindingInfos;
	};

	struct PipelineLibraryArchetype {
		PipelineType type;
		std::vector<VkShaderModule> m_shaders;
		std::vector<uint32_t> m_setLayoutIndices;
		std::vector<VkPushConstantRange> m_pushConstantRanges;
	};

	struct PipelineLibraryInstance {
		PipelineLibraryInstance(const PipelineLibraryInstance& other) = delete;
		PipelineLibraryInstance(PipelineLibraryInstance&& other) = delete;

		uint32_t archetypeID;
		std::vector<VkVertexInputAttributeDescription> attribDescriptions;
		std::vector<VkVertexInputBindingDescription> attribDescriptions;
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

		void create(DeviceContext* deviceContext);

	  private:
		DeviceContext* m_deviceContext;

		std::vector<PipelineLibraryArchetype> m_archetypes;
		std::vector<PipelineLibraryInstance> m_instances;

		std::vector<VkDescriptorSetLayout> m_descriptorSetLayouts;
	};

} // namespace vanadium::graphics