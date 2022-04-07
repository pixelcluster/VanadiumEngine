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
		std::vector<VkDescriptorSetLayoutBinding> bindingInfos;
	};

	struct PipelineLibraryArchetype {
		PipelineType type;
		std::vector<VkShaderModule> shaderModules;
		std::vector<uint32_t> m_setLayoutIndices;
		std::vector<VkPushConstantRange> m_pushConstantRanges;
	};

	struct PipelineLibraryStageSpecialization {
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

	struct AttachmentPassSignature {
		bool isUsed;
		VkFormat format;
		VkSampleCountFlagBits sampleCount;
	};

	struct SubpassSignature {
		std::vector<AttachmentPassSignature> outputAttachments;
		std::vector<AttachmentPassSignature> inputAttachments;
		std::vector<AttachmentPassSignature> resolveAttachments;
		std::vector<AttachmentPassSignature> preserveAttachments;
	};

	struct RenderPassSignature {
		std::vector<SubpassSignature> subpassSignatures;
		std::vector<AttachmentPassSignature> attachmentDescriptionSignatures;
		std::vector<VkSubpassDependency> subpassDependencies;
	};
} // namespace vanadium::graphics

namespace robin_hood {
	template <std::integral T, std::integral U> constexpr size_t hashCombine(T t, U u) {
		if (t == 0)
			return u;
		if (u == 0)
			return t;
		return t ^ (0x517cc1b727220a95 + ((t << 2) ^ (u >> 3)));
	}

	template <std::integral T, std::integral... Ts> constexpr size_t hashCombine(T t, Ts... ts) {
		size_t u = hashCombine(ts...);
		return hashCombine(t, u);
	}

	template <> struct hash<vanadium::graphics::AttachmentPassSignature> {
		size_t operator()(const vanadium::graphics::AttachmentPassSignature& object) const {
			if (!object.isUsed)
				return 0ULL;
			return hashCombine(hash<VkFormat>()(object.format), hash<VkSampleCountFlagBits>()(object.sampleCount));
		}
	};

	template <> struct hash<std::vector<vanadium::graphics::AttachmentPassSignature>> {
		size_t operator()(const std::vector<vanadium::graphics::AttachmentPassSignature>& object) const {
			size_t value = 0;
			for (const auto& signature : object) {
				value = hashCombine(value, hash<vanadium::graphics::AttachmentPassSignature>()(signature));
			}
			return value;
		}
	};

	template <> struct hash<vanadium::graphics::SubpassSignature> {
		size_t operator()(const vanadium::graphics::SubpassSignature& object) const {
			return hashCombine(hash<decltype(object.inputAttachments)>()(object.inputAttachments),
							   hash<decltype(object.outputAttachments)>()(object.outputAttachments),
							   hash<decltype(object.resolveAttachments)>()(object.resolveAttachments),
							   hash<decltype(object.preserveAttachments)>()(object.preserveAttachments));
		}

		size_t operator()(const vanadium::graphics::SubpassSignature& object, bool isSingleSubpass) const {
			if (isSingleSubpass) {
				return hashCombine(hash<decltype(object.inputAttachments)>()(object.inputAttachments),
								   hash<decltype(object.outputAttachments)>()(object.outputAttachments),
								   hash<decltype(object.preserveAttachments)>()(object.preserveAttachments));
			} else {
				return operator()(object);
			}
		}
	};

	template <> struct hash<vanadium::graphics::RenderPassSignature> {
		size_t operator()(const vanadium::graphics::RenderPassSignature& object) const {
			size_t subpassHash = 0;
			if (object.subpassSignatures.size() == 1) {
				subpassHash = hash<vanadium::graphics::SubpassSignature>()(object.subpassSignatures[0], true);
			} else {
				for (const auto& signature : object.subpassSignatures) {
					subpassHash = hashCombine(subpassHash, hash<vanadium::graphics::SubpassSignature>()(signature));
				}
			}
			size_t descriptionHash = hash<std::vector<vanadium::graphics::AttachmentPassSignature>>()(
				object.attachmentDescriptionSignatures);
			size_t dependencyHash = 0;
			for (auto& dependency : object.subpassDependencies) {
				dependencyHash =
					hashCombine(dependencyHash, hash<decltype(dependency.srcAccessMask)>()(dependency.srcAccessMask),
								hash<decltype(dependency.srcStageMask)>()(dependency.srcStageMask),
								hash<decltype(dependency.srcSubpass)>()(dependency.srcSubpass),
								hash<decltype(dependency.dstAccessMask)>()(dependency.dstAccessMask),
								hash<decltype(dependency.dstStageMask)>()(dependency.dstStageMask),
								hash<decltype(dependency.dstSubpass)>()(dependency.dstSubpass),
								hash<decltype(dependency.dependencyFlags)>()(dependency.dependencyFlags));
			}
			return hashCombine(subpassHash, descriptionHash, dependencyHash);
		}
	};
} // namespace robin_hood

namespace vanadium::graphics {
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
		VkViewport viewport;
		VkRect2D scissorRect;
		VkGraphicsPipelineCreateInfo pipelineCreateInfo;
		VkPipelineLayout layout;

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

		void create(const std::string& libraryFileName, DeviceContext* deviceContext);

		void createForPass(const RenderPassSignature& signature, const std::vector<uint32_t>& pipelineIDs);

		// these methods are essentially const but the user can modify state using the pipeline handles
		VkPipeline graphicsPipeline(uint32_t id, const RenderPassSignature& signature) {
			return m_graphicsInstances[id].pipelines[signature];
		}
		VkPipeline computePipeline(uint32_t id) { return m_computeInstances[id].pipeline; }

		std::string_view graphicsPipelineName(uint32_t id) const { return m_graphicsInstances[id].name; }
		std::string_view computePipelineName(uint32_t id) const { return m_computeInstances[id].name; }

	  private:
		DeviceContext* m_deviceContext;
		void* m_buffer;
		size_t m_fileSize;

		template <typename T> T readBuffer(uint64_t& currentOffset);

		VkStencilOpState readStencilState(uint64_t& offset);

		void createGraphicsPipeline(uint64_t& bufferOffset);
		void createComputePipeline(uint64_t& bufferOffset);

		std::vector<PipelineLibraryArchetype> m_archetypes;
		std::vector<PipelineLibraryGraphicsInstance> m_graphicsInstances;
		std::vector<PipelineLibraryComputeInstance> m_computeInstances;

		std::vector<DescriptorSetLayoutInfo> m_descriptorSetLayouts;
		std::vector<VkSampler> m_immutableSamplers;
	};

	template <typename T> T PipelineLibrary::readBuffer(uint64_t& currentOffset) {
		assertFatal(currentOffset + sizeof(T) <= m_fileSize, "PipelineLibrary: Invalid pipeline library file!\n");
		T value;
		std::memcpy(&value, reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(m_buffer) + currentOffset), sizeof(T));

		return value;
	}

} // namespace vanadium::graphics