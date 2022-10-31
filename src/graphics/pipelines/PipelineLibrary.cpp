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

		m_fileStream = std::ifstream(std::string(libraryFileName), std::ios::binary);
		assertFatal(m_fileStream.is_open(), SubsystemID::RHI, "PipelineLibrary: Could not open pipeline file!");

		VCPFileHeader header = deserialize<VCPFileHeader>(m_fileStream);
		assertFatal(header.magic == vcpMagicNumber, SubsystemID::RHI, "PipelineLibrary: Invalid pipeline file!");
		assertFatal(header.version == vcpFileVersion, SubsystemID::RHI, "PipelineLibrary: Invalid pipeline file version!");

		std::vector<std::vector<DescriptorBindingLayoutInfo>> setLayoutInfos;
		setLayoutInfos = deserializeVector<std::vector<DescriptorBindingLayoutInfo>>(m_fileStream);
		m_descriptorSetLayouts.reserve(setLayoutInfos.size());

		uint32_t immutableSamplerOffset = 0;
		for (auto& set : setLayoutInfos) {
			std::vector<VkDescriptorSetLayoutBinding> bindings;
			bindings.reserve(set.size());
			for (auto& binding : set) {
				if (binding.usesImmutableSamplers) {
					for (auto& info : binding.immutableSamplerInfos) {
						VkSamplerCreateInfo createInfo = { .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
														   .magFilter = info.magFilter,
														   .minFilter = info.minFilter,
														   .mipmapMode = info.mipmapMode,
														   .addressModeU = info.addressModeU,
														   .addressModeV = info.addressModeV,
														   .addressModeW = info.addressModeW,
														   .mipLodBias = info.mipLodBias,
														   .anisotropyEnable = info.anisotropyEnable,
														   .maxAnisotropy = info.maxAnisotropy,
														   .compareEnable = info.compareEnable,
														   .compareOp = info.compareOp,
														   .minLod = info.minLod,
														   .maxLod = info.maxLod,
														   .borderColor = info.borderColor,
														   .unnormalizedCoordinates = info.unnormalizedCoordinates };
						VkSampler immutableSampler;
						verifyResult(
							vkCreateSampler(m_deviceContext->device(), &createInfo, nullptr, &immutableSampler));
						m_immutableSamplers.push_back(immutableSampler);
					}
					// dummy value to mark this binding as using immutable samplers
					binding.binding.pImmutableSamplers = m_immutableSamplers.data();
				}
				else {
					binding.binding.pImmutableSamplers = nullptr;
				}
				bindings.push_back(binding.binding);
			}

			for (auto& binding : bindings) {
				if (binding.pImmutableSamplers) {
					binding.pImmutableSamplers = m_immutableSamplers.data() + immutableSamplerOffset;
					immutableSamplerOffset += binding.descriptorCount;
				}
			}

			VkDescriptorSetLayoutCreateInfo createInfo = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
														   .bindingCount = static_cast<uint32_t>(bindings.size()),
														   .pBindings = bindings.data() };
			VkDescriptorSetLayout layout;
			verifyResult(vkCreateDescriptorSetLayout(m_deviceContext->device(), &createInfo, nullptr, &layout));
			m_descriptorSetLayouts.push_back({ .layout = layout, .bindingInfos = std::move(bindings) });
		}

		while (true) {
			PipelineType pipelineType = deserialize<PipelineType>(m_fileStream);

			if (m_fileStream.eof())
				break;

			assertFatal(m_fileStream.good(), SubsystemID::RHI, "PipelineLibrary: Error reading pipeline file!");

			switch (pipelineType) {
				case PipelineType::Graphics:
					createGraphicsPipeline();
					break;
				case PipelineType::Compute:
					createComputePipeline();
					break;
				default:
					logFatal(SubsystemID::RHI, "PipelineLibrary: Invalid pipeline file!");
			}
		}
		m_fileStream.close();
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

	void PipelineLibrary::createGraphicsPipeline() {
		std::vector<CompiledShader> shaders = deserializeVector<CompiledShader>(m_fileStream);
		std::vector<VkPipelineShaderStageCreateInfo> stageCreateInfos;
		std::vector<VkShaderModule> shaderModules;
		stageCreateInfos.reserve(shaders.size());
		shaderModules.reserve(shaders.size());

		for (auto& shader : shaders) {
			VkShaderModuleCreateInfo createInfo = { .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
													.codeSize = shader.dataSize,
													.pCode = reinterpret_cast<uint32_t*>(shader.data) };
			VkShaderModule shaderModule;
			verifyResult(vkCreateShaderModule(m_deviceContext->device(), &createInfo, nullptr, &shaderModule));
			shaderModules.push_back(shaderModule);
			stageCreateInfos.push_back({ .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
										 .stage = shader.stage,
										 .module = shaderModule,
										 .pName = "main" });
		}

		std::vector<VkDescriptorSetLayout> setLayouts;
		std::vector<uint32_t> setLayoutIndices = deserializeVector<uint32_t>(m_fileStream);
		setLayouts.reserve(setLayoutIndices.size());

		for (auto& index : setLayoutIndices) {
			setLayouts.push_back(m_descriptorSetLayouts[index].layout);
		}

		std::vector<VkPushConstantRange> pushConstantRanges = deserializeVector<VkPushConstantRange>(m_fileStream);

		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
																.setLayoutCount =
																	static_cast<uint32_t>(setLayouts.size()),
																.pSetLayouts = setLayouts.data(),
																.pushConstantRangeCount =
																	static_cast<uint32_t>(pushConstantRanges.size()),
																.pPushConstantRanges = pushConstantRanges.data() };
		VkPipelineLayout layout;
		verifyResult(vkCreatePipelineLayout(m_deviceContext->device(), &pipelineLayoutCreateInfo, nullptr, &layout));

		m_archetypes.push_back({ .type = PipelineType::Graphics,
								 .shaderModules = std::move(shaderModules),
								 .setLayoutIndices = std::move(setLayoutIndices),
								 .pushConstantRanges = std::move(pushConstantRanges) });

		std::vector<PipelineInstanceData> fileInstances = deserializeVector<PipelineInstanceData>(m_fileStream);
		std::vector<PipelineLibraryGraphicsInstance> instances;
		instances.reserve(fileInstances.size());

		for (auto& fileInstance : fileInstances) {
			PipelineLibraryGraphicsInstance instance;
			instance.archetypeID = m_archetypes.size() - 1ULL;
			instance.name = fileInstance.name;
			instance.attribDescriptions = fileInstance.instanceVertexInputConfig.attributes;
			instance.bindingDescriptions = fileInstance.instanceVertexInputConfig.bindings;

			instance.vertexInputConfig = { .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
										   .vertexBindingDescriptionCount =
											   static_cast<uint32_t>(instance.bindingDescriptions.size()),
										   .pVertexBindingDescriptions = instance.bindingDescriptions.data(),
										   .vertexAttributeDescriptionCount =
											   static_cast<uint32_t>(instance.attribDescriptions.size()),
										   .pVertexAttributeDescriptions = instance.attribDescriptions.data() };

			instance.inputAssemblyConfig = { .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
											 .topology = fileInstance.instanceInputAssemblyConfig.topology,
											 .primitiveRestartEnable =
												 fileInstance.instanceInputAssemblyConfig.primitiveRestart };

			instance.rasterizationConfig = {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
				.depthClampEnable = fileInstance.instanceRasterizationConfig.depthClampEnable,
				.rasterizerDiscardEnable = fileInstance.instanceRasterizationConfig.rasterizerDiscardEnable,
				.polygonMode = fileInstance.instanceRasterizationConfig.polygonMode,
				.cullMode = fileInstance.instanceRasterizationConfig.cullMode,
				.frontFace = fileInstance.instanceRasterizationConfig.frontFace,
				.depthBiasEnable = fileInstance.instanceRasterizationConfig.depthBiasEnable,
				.depthBiasConstantFactor = fileInstance.instanceRasterizationConfig.depthBiasConstantFactor,
				.depthBiasClamp = fileInstance.instanceRasterizationConfig.depthBiasClamp,
				.depthBiasSlopeFactor = fileInstance.instanceRasterizationConfig.depthBiasSlopeFactor,
				.lineWidth = fileInstance.instanceRasterizationConfig.lineWidth
			};

			instance.viewports = fileInstance.instanceViewportScissorConfig.viewports;
			instance.scissorRects = fileInstance.instanceViewportScissorConfig.scissorRects;

			instance.multisampleConfig = { .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
										   .rasterizationSamples = fileInstance.instanceMultisampleConfig };

			instance.depthStencilConfig = {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
				.depthTestEnable = fileInstance.instanceDepthStencilConfig.depthTestEnable,
				.depthWriteEnable = fileInstance.instanceDepthStencilConfig.depthWriteEnable,
				.depthCompareOp = fileInstance.instanceDepthStencilConfig.depthCompareOp,
				.depthBoundsTestEnable = fileInstance.instanceDepthStencilConfig.depthBoundsTestEnable,
				.stencilTestEnable = fileInstance.instanceDepthStencilConfig.stencilTestEnable,
				.front = fileInstance.instanceDepthStencilConfig.front,
				.back = fileInstance.instanceDepthStencilConfig.back,
				.minDepthBounds = fileInstance.instanceDepthStencilConfig.minDepthBounds,
				.maxDepthBounds = fileInstance.instanceDepthStencilConfig.maxDepthBounds
			};

			instance.dynamicStates = fileInstance.instanceDynamicStateConfig.dynamicStates;

			instance.colorBlendConfig = { .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
										  .logicOpEnable = fileInstance.instanceColorBlendConfig.logicOpEnable,
										  .logicOp = fileInstance.instanceColorBlendConfig.logicOp,
										  .blendConstants = {
											  fileInstance.instanceColorBlendConfig.blendConstants[0],
											  fileInstance.instanceColorBlendConfig.blendConstants[1],
											  fileInstance.instanceColorBlendConfig.blendConstants[2],
											  fileInstance.instanceColorBlendConfig.blendConstants[3] } };

			instance.colorAttachmentBlendConfigs = fileInstance.instanceColorAttachmentBlendConfigs;
			instance.colorBlendConfig.attachmentCount =
				static_cast<uint32_t>(instance.colorAttachmentBlendConfigs.size());
			instance.colorBlendConfig.pAttachments = instance.colorAttachmentBlendConfigs.data();

			for (auto& specialization : fileInstance.instanceSpecializationConfigs) {
				PipelineLibraryStageSpecialization stageSpecialization;
				stageSpecialization.stage = specialization.stage;
				stageSpecialization.mapEntries.reserve(specialization.configs.size());
				size_t dataSize = 0;
				for (auto& config : specialization.configs) {
					stageSpecialization.mapEntries.push_back(config.mapEntry);
					dataSize = std::max(dataSize, config.mapEntry.offset + config.mapEntry.size);
					assertFatal(config.mapEntry.size <= sizeof(float), SubsystemID::RHI, "PipelineLibrary: Invalid pipeline file!");
				}
				uint32_t specializationDataSize = dataSize;
				stageSpecialization.specializationData = new char[specializationDataSize];
				for (auto& config : specialization.configs) {
					std::memcpy(stageSpecialization.specializationData + config.mapEntry.offset, &config.dataUint32,
								config.mapEntry.size);
				}

				stageSpecialization.specializationInfo = { .mapEntryCount = static_cast<uint32_t>(
															   stageSpecialization.mapEntries.size()),
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
										.viewportCount = static_cast<uint32_t>(instance.viewports.size()),
										.pViewports = instance.viewports.data(),
										.scissorCount = static_cast<uint32_t>(instance.scissorRects.size()),
										.pScissors = instance.scissorRects.data() };

			instance.dynamicStateConfig = { .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
											.dynamicStateCount = static_cast<uint32_t>(instance.dynamicStates.size()),
											.pDynamicStates = instance.dynamicStates.data() };

			instance.pipelineCreateInfo =
				VkGraphicsPipelineCreateInfo{ .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
											  .stageCount =
												  static_cast<uint32_t>(instance.shaderStageCreateInfos.size()),
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
				setObjectName(m_deviceContext->device(), VK_OBJECT_TYPE_PIPELINE_LAYOUT,
							  instance.pipelineCreateInfo.layout, instance.name + " Pipeline Layout");
			}

			m_graphicsInstances.push_back(std::move(instance));
		}
	}

	void PipelineLibrary::createComputePipeline() {
		std::vector<CompiledShader> shaders = deserializeVector<CompiledShader>(m_fileStream);
		std::vector<VkShaderModule> shaderModules;
		shaderModules.reserve(shaders.size());

		for (auto& shader : shaders) {
			VkShaderModuleCreateInfo createInfo = { .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
													.codeSize = shader.dataSize,
													.pCode = reinterpret_cast<uint32_t*>(shader.data) };
			VkShaderModule shaderModule;
			verifyResult(vkCreateShaderModule(m_deviceContext->device(), &createInfo, nullptr, &shaderModule));
			shaderModules.push_back(shaderModule);
		}

		assertFatal(shaders.size() == 1, SubsystemID::RHI, "PipelineLibrary: Invalid pipeline file!");

		VkPipelineShaderStageCreateInfo stageInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
													  .stage = VK_SHADER_STAGE_COMPUTE_BIT,
													  .module = shaderModules[0],
													  .pName = "main" };

		std::vector<VkDescriptorSetLayout> setLayouts;
		std::vector<uint32_t> setLayoutIndices = deserializeVector<uint32_t>(m_fileStream);
		setLayouts.reserve(setLayoutIndices.size());

		for (auto& index : setLayoutIndices) {
			setLayouts.push_back(m_descriptorSetLayouts[index].layout);
		}

		std::vector<VkPushConstantRange> pushConstantRanges = deserializeVector<VkPushConstantRange>(m_fileStream);

		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
																.setLayoutCount =
																	static_cast<uint32_t>(setLayouts.size()),
																.pSetLayouts = setLayouts.data(),
																.pushConstantRangeCount =
																	static_cast<uint32_t>(pushConstantRanges.size()),
																.pPushConstantRanges = pushConstantRanges.data() };
		VkPipelineLayout layout;
		verifyResult(vkCreatePipelineLayout(m_deviceContext->device(), &pipelineLayoutCreateInfo, nullptr, &layout));

		m_archetypes.push_back({ .type = PipelineType::Compute,
								 .shaderModules = {},
								 .setLayoutIndices = std::move(setLayoutIndices),
								 .pushConstantRanges = std::move(pushConstantRanges) });

		std::vector<PipelineInstanceData> fileInstances = deserializeVector<PipelineInstanceData>(m_fileStream);
		std::vector<PipelineLibraryGraphicsInstance> instances;
		instances.reserve(fileInstances.size());
		PipelineLibraryStageSpecialization stageSpecialization = {};

		for (auto& fileInstance : fileInstances) {
			PipelineLibraryComputeInstance instance = { .archetypeID =
															static_cast<uint32_t>(m_archetypes.size() - 1ULL),
														.name = fileInstance.name };

			for (auto& specialization : fileInstance.instanceSpecializationConfigs) {
				stageSpecialization.stage = specialization.stage;
				stageSpecialization.mapEntries.reserve(specialization.configs.size());
				size_t dataSize = 0;
				for (auto& config : specialization.configs) {
					stageSpecialization.mapEntries.push_back(config.mapEntry);
					dataSize = std::max(dataSize, config.mapEntry.offset + config.mapEntry.size);
					assertFatal(config.mapEntry.size <= sizeof(float), SubsystemID::RHI, "PipelineLibrary: Invalid pipeline file!");
				}
				uint32_t specializationDataSize = dataSize;
				stageSpecialization.specializationData = new char[specializationDataSize];
				for (auto& config : specialization.configs) {
					std::memcpy(stageSpecialization.specializationData + config.mapEntry.offset, &config.dataUint32,
								config.mapEntry.size);
				}

				stageSpecialization.specializationInfo = { .mapEntryCount = static_cast<uint32_t>(
															   stageSpecialization.mapEntries.size()),
														   .pMapEntries = stageSpecialization.mapEntries.data(),
														   .dataSize = specializationDataSize,
														   .pData = stageSpecialization.specializationData };

				stageInfo.pSpecializationInfo = &stageSpecialization.specializationInfo;
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

			vkDestroyShaderModule(m_deviceContext->device(), shaderModules[0], nullptr);
		}
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
