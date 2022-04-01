#include <execution>
#include <fstream>
#include <graphics/helper/ErrorHelper.hpp>
#include <graphics/pipelines/PipelineLibrary.hpp>
#include <helper/WholeFileReader.hpp>
#include <volk.h>

namespace vanadium::graphics {
	PipelineLibraryGraphicsInstance::PipelineLibraryGraphicsInstance(PipelineLibraryGraphicsInstance&& other)
		: archetypeID(std::forward<decltype(archetypeID)>(other.archetypeID)),
		  shaderStageCreateInfos(std::forward<decltype(shaderStageCreateInfos)>(other.shaderStageCreateInfos)),
		  stageSpecializations(std::forward<decltype(stageSpecializations)>(other.stageSpecializations)),
		  attribDescriptions(std::forward<decltype(attribDescriptions)>(other.attribDescriptions)),
		  bindingDescriptions(std::forward<decltype(bindingDescriptions)>(other.bindingDescriptions)),
		  vertexInputConfig(std::forward<decltype(vertexInputConfig)>(other.vertexInputConfig)),
		  inputAssemblyConfig(std::forward<decltype(inputAssemblyConfig)>(other.inputAssemblyConfig)),
		  rasterizationConfig(std::forward<decltype(rasterizationConfig)>(other.rasterizationConfig)),
		  multisampleConfig(std::forward<decltype(multisampleConfig)>(other.multisampleConfig)),
		  depthStencilConfig(std::forward<decltype(depthStencilConfig)>(other.depthStencilConfig)),
		  colorBlendConfig(std::forward<decltype(colorBlendConfig)>(other.colorBlendConfig)),
		  colorAttachmentBlendConfigs(
			  std::forward<decltype(colorAttachmentBlendConfigs)>(other.colorAttachmentBlendConfigs)),
		  dynamicStateConfig(std::forward<decltype(dynamicStateConfig)>(other.dynamicStateConfig)),
		  dynamicStates(std::forward<decltype(dynamicStates)>(other.dynamicStates)),
		  viewportConfig(std::forward<decltype(viewportConfig)>(other.viewportConfig)),
		  viewport(std::forward<decltype(viewport)>(other.viewport)),
		  scissorRect(std::forward<decltype(scissorRect)>(other.scissorRect)),
		  pipelineCreateInfo(std::forward<decltype(pipelineCreateInfo)>(other.pipelineCreateInfo)),
		  layout(std::forward<decltype(layout)>(other.layout)) {
		vertexInputConfig.pVertexAttributeDescriptions = attribDescriptions.data();
		vertexInputConfig.pVertexBindingDescriptions = bindingDescriptions.data();
		colorBlendConfig.pAttachments = colorAttachmentBlendConfigs.data();
		viewportConfig.pViewports = &viewport;
		viewportConfig.pScissors = &scissorRect;
		dynamicStateConfig.pDynamicStates = dynamicStates.data();
		pipelineCreateInfo.pStages = shaderStageCreateInfos.data();
		pipelineCreateInfo.pVertexInputState = &vertexInputConfig;
		pipelineCreateInfo.pInputAssemblyState = &inputAssemblyConfig;
		pipelineCreateInfo.pRasterizationState = &rasterizationConfig;
		pipelineCreateInfo.pViewportState = &viewportConfig;
		pipelineCreateInfo.pMultisampleState = &multisampleConfig;
		pipelineCreateInfo.pViewportState = &viewportConfig;
		pipelineCreateInfo.pDepthStencilState = &depthStencilConfig;
		pipelineCreateInfo.pColorBlendState = &colorBlendConfig;
		pipelineCreateInfo.pDynamicState = &dynamicStateConfig;

		for (auto& specialization : stageSpecializations) {
			specialization.specializationInfo.pMapEntries = specialization.mapEntries.data();
			auto stageIterator =
				std::find_if(shaderStageCreateInfos.begin(), shaderStageCreateInfos.end(),
							 [&specialization](const auto& stage) { return stage.stage == specialization.stage; });
			stageIterator->pSpecializationInfo = &specialization.specializationInfo;
		}
	}

	void PipelineLibrary::create(const std::string& libraryFileName, DeviceContext* context) {
		m_deviceContext = context;

		m_buffer = readFile(libraryFileName.c_str(), &m_fileSize);
		assertFatal(m_fileSize > 0, "PipelineLibrary: Invalid pipeline library file!\n");
		uint64_t headerReadOffset;
		uint32_t version = readBuffer<uint32_t>(headerReadOffset);

		assertFatal(version == pipelineFileVersion, "PipelineLibrary: Invalid pipeline file version!\n");

		uint32_t pipelineCount = readBuffer<uint32_t>(headerReadOffset);
		uint32_t setLayoutCount = readBuffer<uint32_t>(headerReadOffset);

		std::vector<uint64_t> setLayoutOffsets;
		setLayoutOffsets.reserve(setLayoutCount);
		for (uint32_t i = 0; i < setLayoutCount; ++i) {
			setLayoutOffsets.push_back(readBuffer<uint64_t>(headerReadOffset));
		}

		for (auto& offset : setLayoutOffsets) {
			uint32_t bindingCount = readBuffer<uint32_t>(offset);
			std::vector<VkDescriptorSetLayoutBinding> bindings;
			bindings.reserve(bindingCount);

			for (uint32_t i = 0; i < bindingCount; ++i) {
				VkDescriptorSetLayoutBinding binding = { .binding = readBuffer<uint32_t>(offset),
														 .descriptorType = readBuffer<VkDescriptorType>(offset),
														 .descriptorCount = readBuffer<uint32_t>(offset),
														 .stageFlags = readBuffer<VkShaderStageFlags>(offset) };
				uint32_t immutableSamplerCount = readBuffer<uint32_t>(offset);
				if (immutableSamplerCount > 0) {
					for (uint32_t j = 0; j < immutableSamplerCount; ++j) {
						VkSamplerCreateInfo createInfo = { .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
														   .magFilter = readBuffer<VkFilter>(offset),
														   .minFilter = readBuffer<VkFilter>(offset),
														   .mipmapMode = readBuffer<VkSamplerMipmapMode>(offset),
														   .addressModeU = readBuffer<VkSamplerAddressMode>(offset),
														   .addressModeV = readBuffer<VkSamplerAddressMode>(offset),
														   .addressModeW = readBuffer<VkSamplerAddressMode>(offset),
														   .mipLodBias = readBuffer<float>(offset),
														   .anisotropyEnable = readBuffer<bool>(offset),
														   .maxAnisotropy = readBuffer<float>(offset),
														   .compareEnable = readBuffer<bool>(offset),
														   .compareOp = readBuffer<VkCompareOp>(offset),
														   .minLod = readBuffer<float>(offset),
														   .maxLod = readBuffer<float>(offset),
														   .borderColor = readBuffer<VkBorderColor>(offset),
														   .unnormalizedCoordinates = readBuffer<bool>(offset) };
						VkSampler sampler;
						verifyResult(vkCreateSampler(m_deviceContext->device(), &createInfo, nullptr, &sampler));
						m_immutableSamplers.push_back(sampler);
					}
					binding.pImmutableSamplers =
						m_immutableSamplers.data() + m_immutableSamplers.size() - immutableSamplerCount;
				}
			}

			VkDescriptorSetLayoutCreateInfo createInfo = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
														   .bindingCount = bindingCount,
														   .pBindings = bindings.data() };
			VkDescriptorSetLayout layout;
			verifyResult(vkCreateDescriptorSetLayout(m_deviceContext->device(), &createInfo, nullptr, &layout));
			m_descriptorSetLayouts.push_back(layout);
		}

		headerReadOffset = setLayoutOffsets.back();

		std::vector<uint64_t> pipelineDataOffsets;
		pipelineDataOffsets.reserve(pipelineCount);
		for (size_t i = 0; i < pipelineCount; ++i) {
			pipelineDataOffsets.push_back(readBuffer<uint64_t>(headerReadOffset));
		}

		for (auto& pipelineOffset : pipelineDataOffsets) {
			PipelineType pipelineType = readBuffer<PipelineType>(pipelineOffset);
			switch (pipelineType) {
				case PipelineType::Graphics:
					createGraphicsPipeline(pipelineOffset);
					break;
				case PipelineType::Compute:
					createComputePipeline(pipelineOffset);
					break;
			}
		}
	}

	void PipelineLibrary::createGraphicsPipeline(uint64_t& bufferOffset) {
		uint32_t shaderCount = readBuffer<uint32_t>(bufferOffset);
		std::vector<VkPipelineShaderStageCreateInfo> stageCreateInfos;
		stageCreateInfos.reserve(shaderCount);

		for (uint32_t i = 0; i < shaderCount; ++i) {
			VkShaderStageFlags stageFlags = readBuffer<uint32_t>(bufferOffset);
			uint32_t shaderSize = readBuffer<uint32_t>(bufferOffset);
			assertFatal(bufferOffset + shaderSize < m_fileSize, "PipelineLibrary: Invalid pipeline library file!\n");
			VkShaderModuleCreateInfo createInfo = { .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
													.codeSize = shaderSize,
													.pCode = reinterpret_cast<uint32_t*>(
														reinterpret_cast<uintptr_t>(m_buffer) + bufferOffset) };
			VkShaderModule shaderModule;
			verifyResult(vkCreateShaderModule(m_deviceContext->device(), &createInfo, nullptr, &shaderModule));

			bufferOffset = shaderSize;

			stageCreateInfos.push_back({ .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
										 .module = shaderModule,
										 .pName = "main" });
		}

		uint32_t setLayoutCount = readBuffer<uint32_t>(bufferOffset);
		std::vector<VkDescriptorSetLayout> setLayouts;
		setLayouts.reserve(setLayoutCount);

		for (uint32_t i = 0; i < setLayoutCount; ++i) {
			setLayouts.push_back(m_descriptorSetLayouts[readBuffer<uint32_t>(bufferOffset)]);
		}

		uint32_t pushConstantRangeCount = readBuffer<uint32_t>(bufferOffset);
		std::vector<VkPushConstantRange> pushConstantRanges;
		pushConstantRanges.reserve(pushConstantRangeCount);

		for (uint32_t i = 0; i < pushConstantRangeCount; ++i) {
			pushConstantRanges.push_back({ .stageFlags = readBuffer<VkShaderStageFlags>(bufferOffset),
										   .offset = readBuffer<uint32_t>(bufferOffset),
										   .size = readBuffer<uint32_t>(bufferOffset) });
		}

		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
																.setLayoutCount = setLayoutCount,
																.pSetLayouts = setLayouts.data(),
																.pushConstantRangeCount = pushConstantRangeCount,
																.pPushConstantRanges = pushConstantRanges.data() };
		VkPipelineLayout layout;
		verifyResult(vkCreatePipelineLayout(m_deviceContext->device(), &pipelineLayoutCreateInfo, nullptr, &layout));

		uint32_t instanceCount = readBuffer<uint32_t>(bufferOffset);
		std::vector<uint64_t> instanceOffsets;
		std::vector<std::string> instanceNames;
		std::vector<PipelineLibraryGraphicsInstance> instances;
		instanceOffsets.reserve(instanceCount);

		for (uint32_t i = 0; i < instanceCount; ++i) {
			instanceOffsets.push_back(readBuffer<uint32_t>(bufferOffset));
		}

		for (auto& offset : instanceOffsets) {
			PipelineLibraryGraphicsInstance instance;

			uint32_t nameSize = readBuffer<uint32_t>(offset);
			std::string name = std::string(nameSize, ' ');
			assertFatal(offset + nameSize < m_fileSize, "PipelineLibrary: Invalid pipeline library file!\n");
			std::memcpy(name.data(), reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(m_buffer) + offset), nameSize);

			uint32_t attributeCount = readBuffer<uint32_t>(offset);
			instance.attribDescriptions.reserve(attributeCount);
			for (uint32_t i = 0; i < attributeCount; ++i) {
				instance.attribDescriptions.push_back({ .location = readBuffer<uint32_t>(offset),
														.binding = readBuffer<uint32_t>(offset),
														.format = readBuffer<VkFormat>(offset),
														.offset = readBuffer<uint32_t>(offset) });
			}

			uint32_t bindingCount = readBuffer<uint32_t>(offset);
			instance.bindingDescriptions.reserve(bindingCount);
			for (uint32_t i = 0; i < bindingCount; ++i) {
				instance.bindingDescriptions.push_back(
					VkVertexInputBindingDescription{ .binding = readBuffer<uint32_t>(offset),
													 .stride = readBuffer<uint32_t>(offset),
													 .inputRate = readBuffer<VkVertexInputRate>(offset) });
			}

			instance.vertexInputConfig = { .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
										   .vertexBindingDescriptionCount = bindingCount,
										   .pVertexBindingDescriptions = instance.bindingDescriptions.data(),
										   .vertexAttributeDescriptionCount = attributeCount,
										   .pVertexAttributeDescriptions = instance.attribDescriptions.data() };

			instance.inputAssemblyConfig = { .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
											 .topology = readBuffer<VkPrimitiveTopology>(offset),
											 .primitiveRestartEnable = readBuffer<bool>(offset) };

			instance.rasterizationConfig = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
											 .depthClampEnable = readBuffer<bool>(offset),
											 .rasterizerDiscardEnable = readBuffer<bool>(offset),
											 .polygonMode = readBuffer<VkPolygonMode>(offset),
											 .cullMode = readBuffer<uint32_t>(offset),
											 .frontFace = readBuffer<VkFrontFace>(offset),
											 .depthBiasEnable = readBuffer<bool>(offset),
											 .depthBiasConstantFactor = readBuffer<float>(offset),
											 .depthBiasClamp = readBuffer<float>(offset),
											 .depthBiasSlopeFactor = readBuffer<float>(offset),
											 .lineWidth = readBuffer<float>(offset) };

			instance.multisampleConfig = { .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
										   .rasterizationSamples = readBuffer<VkSampleCountFlagBits>(offset) };

			instance.depthStencilConfig = { .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
											.depthTestEnable = readBuffer<bool>(offset),
											.depthWriteEnable = readBuffer<bool>(offset),
											.depthCompareOp = readBuffer<VkCompareOp>(offset),
											.depthBoundsTestEnable = readBuffer<bool>(offset),
											.stencilTestEnable = readBuffer<bool>(offset),
											.front = readStencilState(offset),
											.back = readStencilState(offset),
											.minDepthBounds = readBuffer<float>(offset),
											.maxDepthBounds = readBuffer<float>(offset) };

			instance.colorBlendConfig = { .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
										  .logicOpEnable = readBuffer<bool>(offset),
										  .logicOp = readBuffer<VkLogicOp>(offset),
										  .blendConstants = { readBuffer<float>(offset), readBuffer<float>(offset),
															  readBuffer<float>(offset), readBuffer<float>(offset) } };

			uint32_t attachmentCount = readBuffer<uint32_t>(offset);
			instance.colorAttachmentBlendConfigs.reserve(attachmentCount);

			for (uint32_t i = 0; i < attachmentCount; ++i) {
				instance.colorAttachmentBlendConfigs.push_back(
					{ .blendEnable = readBuffer<bool>(offset),
					  .srcColorBlendFactor = readBuffer<VkBlendFactor>(offset),
					  .dstColorBlendFactor = readBuffer<VkBlendFactor>(offset),
					  .colorBlendOp = readBuffer<VkBlendOp>(offset),
					  .srcAlphaBlendFactor = readBuffer<VkBlendFactor>(offset),
					  .dstAlphaBlendFactor = readBuffer<VkBlendFactor>(offset),
					  .alphaBlendOp = readBuffer<VkBlendOp>(offset),
					  .colorWriteMask = readBuffer<VkColorComponentFlags>(offset) });
			}
			instance.colorBlendConfig.attachmentCount = attachmentCount;
			instance.colorBlendConfig.pAttachments = instance.colorAttachmentBlendConfigs.data();

			uint32_t specializationStageCount = readBuffer<uint32_t>(offset);
			for (uint32_t i = 0; i < specializationStageCount; ++i) {
				PipelineLibraryStageSpecialization stageSpecialization;
				stageSpecialization.stage = readBuffer<VkShaderStageFlagBits>(offset);
				uint32_t specializationMapEntryCount = readBuffer<uint32_t>(offset);
				stageSpecialization.mapEntries.reserve(specializationMapEntryCount);
				for (uint32_t i = 0; i < specializationMapEntryCount; ++i) {
					stageSpecialization.mapEntries.push_back({ .constantID = readBuffer<uint32_t>(offset),
															   .offset = readBuffer<uint32_t>(offset),
															   .size = readBuffer<uint32_t>(offset) });
				}
				uint32_t specializationDataSize = readBuffer<uint32_t>(offset);
				assertFatal(offset + specializationDataSize <= m_fileSize, "Invalid pipeline library file!\n");
				stageSpecialization.specializationData = new char[specializationDataSize];
				std::memcpy(stageSpecialization.specializationData,
							reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(m_buffer) + offset),
							specializationDataSize);

				stageSpecialization.specializationInfo = { .mapEntryCount = specializationMapEntryCount,
														   .pMapEntries = stageSpecialization.mapEntries.data(),
														   .dataSize = specializationDataSize,
														   .pData = stageSpecialization.specializationData };
				instance.stageSpecializations.push_back(std::move(stageSpecialization));

				auto stageIterator = std::find_if(
					instance.shaderStageCreateInfos.begin(), instance.shaderStageCreateInfos.end(),
					[&stageSpecialization](const auto& stage) { return stage.stage == stageSpecialization.stage; });
				stageIterator->pSpecializationInfo = &instance.stageSpecializations.back().specializationInfo;
			}

			instance.viewportConfig = { .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
										.viewportCount = 1,
										.pViewports = &instance.viewport,
										.scissorCount = 1,
										.pScissors = &instance.scissorRect };

			instance.dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT };

			instance.dynamicStateConfig = { .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
											.dynamicStateCount = static_cast<uint32_t>(instance.dynamicStates.size()),
											.pDynamicStates = instance.dynamicStates.data() };

			instance.pipelineCreateInfo =
				VkGraphicsPipelineCreateInfo{ .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
											  .stageCount = shaderCount,
											  .pStages = instance.shaderStageCreateInfos.data(),
											  .pVertexInputState = &instance.vertexInputConfig,
											  .pInputAssemblyState = &instance.inputAssemblyConfig,
											  .pViewportState = &instance.viewportConfig,
											  .pRasterizationState = &instance.rasterizationConfig,
											  .pMultisampleState = &instance.multisampleConfig,
											  .pDepthStencilState = &instance.depthStencilConfig,
											  .pColorBlendState = &instance.colorBlendConfig,
											  .pDynamicState = &instance.dynamicStateConfig,
											  .layout = layout };

			m_graphicsInstanceNames.insert(robin_hood::pair<const std::string, PipelineLibraryGraphicsInstance>(
				std::move(name), std::move(instance)));
		}
	}

	void PipelineLibrary::createComputePipeline(uint64_t& bufferOffset) {
		uint32_t shaderCount = readBuffer<uint32_t>(bufferOffset);

		VkShaderStageFlags stageFlags = readBuffer<uint32_t>(bufferOffset);
		uint32_t shaderSize = readBuffer<uint32_t>(bufferOffset);
		assertFatal(bufferOffset + shaderSize < m_fileSize, "PipelineLibrary: Invalid pipeline library file!\n");
		VkShaderModuleCreateInfo createInfo = { .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
												.codeSize = shaderSize,
												.pCode = reinterpret_cast<uint32_t*>(
													reinterpret_cast<uintptr_t>(m_buffer) + bufferOffset) };
		VkShaderModule shaderModule;
		verifyResult(vkCreateShaderModule(m_deviceContext->device(), &createInfo, nullptr, &shaderModule));

		bufferOffset = shaderSize;

		VkPipelineShaderStageCreateInfo stageInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
													  .module = shaderModule,
													  .pName = "main" };

		uint32_t setLayoutCount = readBuffer<uint32_t>(bufferOffset);
		std::vector<VkDescriptorSetLayout> setLayouts;
		setLayouts.reserve(setLayoutCount);

		for (uint32_t i = 0; i < setLayoutCount; ++i) {
			setLayouts.push_back(m_descriptorSetLayouts[readBuffer<uint32_t>(bufferOffset)]);
		}

		uint32_t pushConstantRangeCount = readBuffer<uint32_t>(bufferOffset);
		std::vector<VkPushConstantRange> pushConstantRanges;
		pushConstantRanges.reserve(pushConstantRangeCount);

		for (uint32_t i = 0; i < pushConstantRangeCount; ++i) {
			pushConstantRanges.push_back({ .stageFlags = readBuffer<VkShaderStageFlags>(bufferOffset),
										   .offset = readBuffer<uint32_t>(bufferOffset),
										   .size = readBuffer<uint32_t>(bufferOffset) });
		}

		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
																.setLayoutCount = setLayoutCount,
																.pSetLayouts = setLayouts.data(),
																.pushConstantRangeCount = pushConstantRangeCount,
																.pPushConstantRanges = pushConstantRanges.data() };
		VkPipelineLayout layout;
		verifyResult(vkCreatePipelineLayout(m_deviceContext->device(), &pipelineLayoutCreateInfo, nullptr, &layout));

		uint32_t instanceCount = readBuffer<uint32_t>(bufferOffset);
		std::vector<uint64_t> instanceOffsets;
		std::vector<std::string> instanceNames;
		std::vector<PipelineLibraryGraphicsInstance> instances;
		instanceOffsets.reserve(instanceCount);

		for (uint32_t i = 0; i < instanceCount; ++i) {
			instanceOffsets.push_back(readBuffer<uint32_t>(bufferOffset));
		}

		VkSpecializationInfo specializationInfo;
		std::vector<VkSpecializationMapEntry> mapEntries;
		char* specializationData;

		for (auto& offset : instanceOffsets) {
			PipelineLibraryComputeInstance instance;

			uint32_t nameSize = readBuffer<uint32_t>(offset);
			std::string name = std::string(nameSize, ' ');
			assertFatal(offset + nameSize < m_fileSize, "PipelineLibrary: Invalid pipeline library file!\n");
			std::memcpy(name.data(), reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(m_buffer) + offset), nameSize);

			uint32_t specializationStageCount = readBuffer<uint32_t>(offset);
			for (uint32_t i = 0; i < specializationStageCount; ++i) {
				assertFatal(readBuffer<VkShaderStageFlagBits>(offset) == VK_SHADER_STAGE_COMPUTE_BIT,
							"PipelineLibrary: Invalid pipeline library file!\n");
				uint32_t specializationMapEntryCount = readBuffer<uint32_t>(offset);
				mapEntries.reserve(specializationMapEntryCount);
				for (uint32_t i = 0; i < specializationMapEntryCount; ++i) {
					mapEntries.push_back({ .constantID = readBuffer<uint32_t>(offset),
										   .offset = readBuffer<uint32_t>(offset),
										   .size = readBuffer<uint32_t>(offset) });
				}
				uint32_t specializationDataSize = readBuffer<uint32_t>(offset);
				assertFatal(offset + specializationDataSize <= m_fileSize, "Invalid pipeline library file!\n");
				specializationData = new char[specializationDataSize];
				std::memcpy(specializationData, reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(m_buffer) + offset),
							specializationDataSize);

				specializationInfo = { .mapEntryCount = specializationMapEntryCount,
									   .pMapEntries = mapEntries.data(),
									   .dataSize = specializationDataSize,
									   .pData = specializationData };

				stageInfo.pSpecializationInfo = &specializationInfo;
			}

			VkComputePipelineCreateInfo computeCreateInfo = { .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
															  .stage = stageInfo,
															  .layout = layout };
			verifyResult(vkCreateComputePipelines(m_deviceContext->device(), VK_NULL_HANDLE, 1, &computeCreateInfo,
												  nullptr, &instance.pipeline));
			instance.layout = layout;
			m_computeInstanceNames.insert(robin_hood::pair<const std::string, PipelineLibraryComputeInstance>(
				std::move(name), std::move(instance)));

			vkDestroyShaderModule(m_deviceContext->device(), shaderModule, nullptr);
			delete[] specializationData;
		}
	}

	VkStencilOpState PipelineLibrary::readStencilState(uint64_t& offset) {
		return { .failOp = readBuffer<VkStencilOp>(offset),
				 .passOp = readBuffer<VkStencilOp>(offset),
				 .depthFailOp = readBuffer<VkStencilOp>(offset),
				 .compareOp = readBuffer<VkCompareOp>(offset),
				 .compareMask = readBuffer<uint32_t>(offset),
				 .writeMask = readBuffer<uint32_t>(offset),
				 .reference = readBuffer<uint32_t>(offset) };
	}
} // namespace vanadium::graphics
