#include <execution>
#include <fstream>
#include <graphics/helper/DebugHelper.hpp>
#include <graphics/helper/ErrorHelper.hpp>
#include <graphics/pipelines/PipelineLibrary.hpp>
#include <util/WholeFileReader.hpp>
#include <volk.h>

namespace vanadium::graphics {
	PipelineLibraryGraphicsInstance::PipelineLibraryGraphicsInstance(PipelineLibraryGraphicsInstance&& other)
		: archetypeID(std::forward<decltype(archetypeID)>(other.archetypeID)),
		  name(std::forward<decltype(name)>(other.name)),
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
		  viewports(std::forward<decltype(viewports)>(other.viewports)),
		  scissorRects(std::forward<decltype(scissorRects)>(other.scissorRects)),
		  pipelineCreateInfo(std::forward<decltype(pipelineCreateInfo)>(other.pipelineCreateInfo)) {
		vertexInputConfig.pVertexAttributeDescriptions = attribDescriptions.data();
		vertexInputConfig.pVertexBindingDescriptions = bindingDescriptions.data();
		colorBlendConfig.pAttachments = colorAttachmentBlendConfigs.data();
		viewportConfig.pViewports = viewports.data();
		viewportConfig.pScissors = scissorRects.data();
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

	void PipelineLibrary::create(const std::string_view& libraryFileName, DeviceContext* context) {
		m_deviceContext = context;

		m_fileSize = 5;
		m_buffer = readFile(libraryFileName.data(), &m_fileSize);
		assertFatal(m_fileSize > 0, "PipelineLibrary: Could not open pipeline file!");
		uint64_t headerReadOffset = 0;
		uint32_t version = readBuffer<uint32_t>(headerReadOffset);

		assertFatal(version == pipelineFileVersion, "PipelineLibrary: Invalid pipeline file version!");

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

			size_t immutableSamplerOffset = m_immutableSamplers.size();

			for (uint32_t i = 0; i < bindingCount; ++i) {
				VkDescriptorSetLayoutBinding binding = { .binding = readBuffer<uint32_t>(offset),
														 .descriptorType = readBuffer<VkDescriptorType>(offset),
														 .descriptorCount = readBuffer<uint32_t>(offset),
														 .stageFlags = readBuffer<VkShaderStageFlags>(offset) };
				bool usesImmutableSamplers = readBuffer<bool>(offset);
				uint32_t immutableSamplerCount = readBuffer<uint32_t>(offset);
				if (usesImmutableSamplers) {
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
					// dummy value, for later, data() might get invalidated
					binding.pImmutableSamplers = m_immutableSamplers.data();
				}
				bindings.push_back(binding);
			}

			for (auto& binding : bindings) {
				if (binding.pImmutableSamplers != nullptr) {
					binding.pImmutableSamplers = m_immutableSamplers.data() + immutableSamplerOffset;
					immutableSamplerOffset += binding.descriptorCount;
				}
			}

			VkDescriptorSetLayoutCreateInfo createInfo = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
														   .bindingCount = bindingCount,
														   .pBindings = bindings.data() };
			VkDescriptorSetLayout layout;
			verifyResult(vkCreateDescriptorSetLayout(m_deviceContext->device(), &createInfo, nullptr, &layout));
			m_descriptorSetLayouts.push_back({ .layout = layout, .bindingInfos = std::move(bindings) });
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
		delete[] reinterpret_cast<char*>(m_buffer);
	}

	void PipelineLibrary::createForPass(const RenderPassSignature& signature, VkRenderPass pass,
										const std::vector<uint32_t>& pipelineIDs) {
		std::for_each(std::execution::par_unseq, pipelineIDs.begin(), pipelineIDs.end(),
					  [this, &signature, pass](const auto& id) {
						  m_graphicsInstances[id].pipelineCreateInfo.renderPass = pass;
						  VkPipeline pipeline;
						  verifyResult(vkCreateGraphicsPipelines(m_deviceContext->device(), VK_NULL_HANDLE, 1,
																 &m_graphicsInstances[id].pipelineCreateInfo, nullptr,
																 &pipeline));

						  if constexpr (vanadiumGPUDebug) {
							  setObjectName(m_deviceContext->device(), VK_OBJECT_TYPE_PIPELINE, pipeline,
											m_graphicsInstances[id].name + " (Signature hash " +
												std::to_string(robin_hood::hash<RenderPassSignature>()(signature)) +
												")");
						  }
						  m_graphicsInstances[id].pipelines.insert(
							  robin_hood::pair<const RenderPassSignature, VkPipeline>(signature, pipeline));
						  m_graphicsInstances[id].pipelineCreateInfo.renderPass = VK_NULL_HANDLE;
					  });
	}

	void PipelineLibrary::createGraphicsPipeline(uint64_t& bufferOffset) {
		uint32_t shaderCount = readBuffer<uint32_t>(bufferOffset);
		std::vector<VkPipelineShaderStageCreateInfo> stageCreateInfos;
		std::vector<VkShaderModule> shaderModules;
		stageCreateInfos.reserve(shaderCount);
		shaderModules.reserve(shaderCount);

		for (uint32_t i = 0; i < shaderCount; ++i) {
			VkShaderStageFlagBits stageFlags = readBuffer<VkShaderStageFlagBits>(bufferOffset);
			uint32_t shaderSize = readBuffer<uint32_t>(bufferOffset);
			assertFatal(bufferOffset + shaderSize < m_fileSize, "PipelineLibrary: Invalid pipeline library file!");
			VkShaderModuleCreateInfo createInfo = { .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
													.codeSize = shaderSize,
													.pCode = reinterpret_cast<uint32_t*>(
														reinterpret_cast<uintptr_t>(m_buffer) + bufferOffset) };
			VkShaderModule shaderModule;
			verifyResult(vkCreateShaderModule(m_deviceContext->device(), &createInfo, nullptr, &shaderModule));

			bufferOffset += shaderSize;

			shaderModules.push_back(shaderModule);
			stageCreateInfos.push_back({ .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
										 .stage = stageFlags,
										 .module = shaderModule,
										 .pName = "main" });
		}

		uint32_t setLayoutCount = readBuffer<uint32_t>(bufferOffset);
		std::vector<VkDescriptorSetLayout> setLayouts;
		std::vector<uint32_t> setLayoutIndices;
		setLayouts.reserve(setLayoutCount);
		setLayoutIndices.reserve(setLayoutCount);

		for (uint32_t i = 0; i < setLayoutCount; ++i) {
			setLayoutIndices.push_back(readBuffer<uint32_t>(bufferOffset));
			setLayouts.push_back(m_descriptorSetLayouts[setLayoutIndices.back()].layout);
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

		m_archetypes.push_back({ .type = PipelineType::Graphics,
								 .shaderModules = std::move(shaderModules),
								 .setLayoutIndices = std::move(setLayoutIndices),
								 .pushConstantRanges = std::move(pushConstantRanges) });

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
			instance.archetypeID = m_archetypes.size() - 1ULL;

			uint32_t nameSize = readBuffer<uint32_t>(offset);
			std::string name = std::string(nameSize, ' ');
			assertFatal(offset + nameSize < m_fileSize, "PipelineLibrary: Invalid pipeline library file!");
			std::memcpy(name.data(), reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(m_buffer) + offset), nameSize);
			offset += nameSize;

			instance.name = std::move(name);

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

			uint32_t viewportCount = readBuffer<uint32_t>(offset);
			instance.viewports.reserve(viewportCount);
			for (uint32_t i = 0; i < viewportCount; ++i) {
				instance.viewports.push_back({ .x = readBuffer<float>(offset),
											   .y = readBuffer<float>(offset),
											   .width = readBuffer<float>(offset),
											   .height = readBuffer<float>(offset),
											   .minDepth = readBuffer<float>(offset),
											   .maxDepth = readBuffer<float>(offset) });
			}

			uint32_t scissorRectCount = readBuffer<uint32_t>(offset);
			instance.scissorRects.reserve(scissorRectCount);
			for (uint32_t i = 0; i < viewportCount; ++i) {
				instance.scissorRects.push_back(
					{ .offset = { .x = readBuffer<int32_t>(offset), .y = readBuffer<int32_t>(offset) },
					  .extent = { .width = readBuffer<uint32_t>(offset), .height = readBuffer<uint32_t>(offset) } });
			}

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

			uint32_t dynamicStateCount = readBuffer<uint32_t>(offset);
			instance.dynamicStates.reserve(dynamicStateCount);
			for(uint32_t i = 0; i < dynamicStateCount; ++i) {
				instance.dynamicStates.push_back(readBuffer<VkDynamicState>(offset));
			}

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
				assertFatal(offset + specializationDataSize <= m_fileSize, "Invalid pipeline library file!");
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

			instance.shaderStageCreateInfos = stageCreateInfos;

			instance.viewportConfig = { .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
										.viewportCount = viewportCount,
										.pViewports = instance.viewports.data(),
										.scissorCount = scissorRectCount,
										.pScissors = instance.scissorRects.data() };

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

			if constexpr (vanadiumGPUDebug) {
				setObjectName(m_deviceContext->device(), VK_OBJECT_TYPE_PIPELINE_LAYOUT, instance.pipelineCreateInfo.layout,
							  instance.name + " Pipeline Layout");
			}

			m_graphicsInstances.push_back(std::move(instance));
		}
	}

	void PipelineLibrary::createComputePipeline(uint64_t& bufferOffset) {
		VkShaderStageFlagBits stage = readBuffer<VkShaderStageFlagBits>(bufferOffset);
		assertFatal(stage == VK_SHADER_STAGE_COMPUTE_BIT, "PipelineLibrary: Invalid pipeline library file!");
		uint32_t shaderSize = readBuffer<uint32_t>(bufferOffset);
		assertFatal(bufferOffset + shaderSize < m_fileSize, "PipelineLibrary: Invalid pipeline library file!");
		VkShaderModuleCreateInfo createInfo = { .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
												.codeSize = shaderSize,
												.pCode = reinterpret_cast<uint32_t*>(
													reinterpret_cast<uintptr_t>(m_buffer) + bufferOffset) };
		VkShaderModule shaderModule;
		verifyResult(vkCreateShaderModule(m_deviceContext->device(), &createInfo, nullptr, &shaderModule));

		bufferOffset += shaderSize;

		VkPipelineShaderStageCreateInfo stageInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
													  .stage = stage,
													  .module = shaderModule,
													  .pName = "main" };

		uint32_t setLayoutCount = readBuffer<uint32_t>(bufferOffset);
		std::vector<VkDescriptorSetLayout> setLayouts;
		std::vector<uint32_t> setLayoutIndices;
		setLayouts.reserve(setLayoutCount);
		setLayoutIndices.reserve(setLayoutCount);

		for (uint32_t i = 0; i < setLayoutCount; ++i) {
			setLayoutIndices.push_back(readBuffer<uint32_t>(bufferOffset));
			setLayouts.push_back(m_descriptorSetLayouts[setLayoutIndices.back()].layout);
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

		m_archetypes.push_back({ .type = PipelineType::Compute,
								 .shaderModules = {},
								 .setLayoutIndices = std::move(setLayoutIndices),
								 .pushConstantRanges = std::move(pushConstantRanges) });

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
		char* specializationData = nullptr;

		for (auto& offset : instanceOffsets) {
			PipelineLibraryComputeInstance instance = { .archetypeID =
															static_cast<uint32_t>(m_archetypes.size() - 1ULL) };

			uint32_t nameSize = readBuffer<uint32_t>(offset);
			instance.name = std::string(nameSize, ' ');
			assertFatal(offset + nameSize < m_fileSize, "PipelineLibrary: Invalid pipeline library file!");
			std::memcpy(instance.name.data(), reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(m_buffer) + offset),
						nameSize);
			offset += nameSize;

			uint32_t specializationStageCount = readBuffer<uint32_t>(offset);
			for (uint32_t i = 0; i < specializationStageCount; ++i) {
				assertFatal(readBuffer<VkShaderStageFlagBits>(offset) == VK_SHADER_STAGE_COMPUTE_BIT,
							"PipelineLibrary: Invalid pipeline library file!");
				uint32_t specializationMapEntryCount = readBuffer<uint32_t>(offset);
				mapEntries.reserve(specializationMapEntryCount);
				for (uint32_t i = 0; i < specializationMapEntryCount; ++i) {
					mapEntries.push_back({ .constantID = readBuffer<uint32_t>(offset),
										   .offset = readBuffer<uint32_t>(offset),
										   .size = readBuffer<uint32_t>(offset) });
				}
				uint32_t specializationDataSize = readBuffer<uint32_t>(offset);
				assertFatal(offset + specializationDataSize <= m_fileSize, "Invalid pipeline library file!");
				specializationData = new char[specializationDataSize];
				std::memcpy(specializationData, reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(m_buffer) + offset),
							specializationDataSize);

				specializationInfo = { .mapEntryCount = specializationMapEntryCount,
									   .pMapEntries = mapEntries.data(),
									   .dataSize = specializationDataSize,
									   .pData = specializationData };

				stageInfo.pSpecializationInfo = &specializationInfo;
			}

			VkComputePipelineCreateInfo computeCreateInfo = { .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
															  .stage = stageInfo,
															  .layout = layout };
			verifyResult(vkCreateComputePipelines(m_deviceContext->device(), VK_NULL_HANDLE, 1, &computeCreateInfo,
												  nullptr, &instance.pipeline));
			instance.layout = layout;

			if constexpr (vanadiumGPUDebug) {
				setObjectName(m_deviceContext->device(), VK_OBJECT_TYPE_PIPELINE, instance.pipeline, instance.name);
				setObjectName(m_deviceContext->device(), VK_OBJECT_TYPE_PIPELINE_LAYOUT, instance.layout,
							  instance.name + " Pipeline Layout");
			}

			m_computeInstances.push_back(std::move(instance));

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

	std::vector<DescriptorSetLayoutInfo> PipelineLibrary::graphicsPipelineSets(uint32_t id) {
		std::vector<DescriptorSetLayoutInfo> result;
		result.reserve(m_archetypes[m_graphicsInstances[id].archetypeID].setLayoutIndices.size());
		for (auto& index : m_archetypes[m_graphicsInstances[id].archetypeID].setLayoutIndices) {
			result.push_back(m_descriptorSetLayouts[index]);
		}
		return result;
	}

	std::vector<DescriptorSetLayoutInfo> PipelineLibrary::computePipelineSets(uint32_t id) {
		std::vector<DescriptorSetLayoutInfo> result;
		result.reserve(m_archetypes[m_computeInstances[id].archetypeID].setLayoutIndices.size());
		for (auto& index : m_archetypes[m_computeInstances[id].archetypeID].setLayoutIndices) {
			result.push_back(m_descriptorSetLayouts[index]);
		}
		return result;
	}

	uint32_t PipelineLibrary::findGraphicsPipeline(const std::string_view& name) {
		auto iterator = std::find_if(m_graphicsInstances.begin(), m_graphicsInstances.end(),
									 [name](const auto& instance) { return instance.name == name; });
		if (iterator == m_graphicsInstances.end())
			return ~0U;
		return iterator - m_graphicsInstances.begin();
	}
	uint32_t PipelineLibrary::findComputePipeline(const std::string_view& name) {
		auto iterator = std::find_if(m_computeInstances.begin(), m_computeInstances.end(),
									 [name](const auto& instance) { return instance.name == name; });
		if (iterator == m_computeInstances.end())
			return ~0U;
		return iterator - m_computeInstances.begin();
	}

	void PipelineLibrary::destroy() {
		for (auto& sampler : m_immutableSamplers) {
			vkDestroySampler(m_deviceContext->device(), sampler, nullptr);
		}
		for (auto& layout : m_descriptorSetLayouts) {
			vkDestroyDescriptorSetLayout(m_deviceContext->device(), layout.layout, nullptr);
		}
		for (auto& archetype : m_archetypes) {
			for (auto& shader : archetype.shaderModules) {
				vkDestroyShaderModule(m_deviceContext->device(), shader, nullptr);
			}
		}
		for (auto& instance : m_graphicsInstances) {
			vkDestroyPipelineLayout(m_deviceContext->device(), instance.pipelineCreateInfo.layout, nullptr);
			for (auto& signaturePair : instance.pipelines) {
				vkDestroyPipeline(m_deviceContext->device(), signaturePair.second, nullptr);
			}
		}
		for (auto& instance : m_computeInstances) {
			vkDestroyPipelineLayout(m_deviceContext->device(), instance.layout, nullptr);
			vkDestroyPipeline(m_deviceContext->device(), instance.pipeline, nullptr);
		}
	}
} // namespace vanadium::graphics
