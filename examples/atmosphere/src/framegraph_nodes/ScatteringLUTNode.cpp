#include <framegraph_nodes/ScatteringLUTNode.hpp>
#include <helper/WholeFileReader.hpp>
#include <modules/gpu/helper/ErrorHelper.hpp>
#include <volk.h>

void ScatteringLUTNode::create(VFramegraphContext* context) {
	VkImageSubresourceRange lutSubresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
													.baseMipLevel = 0,
													.levelCount = 1,
													.baseArrayLayer = 0,
													.layerCount = 1 };
	m_transmittanceLUTHandle = context->declareTransientImage(
		this,
		{ .imageType = VK_IMAGE_TYPE_2D,
		  .format = VK_FORMAT_R32G32B32A32_SFLOAT,
		  .extent = { .width = 256, .height = 64, .depth = 1 },
		  .mipLevels = 1,
		  .arrayLayers = 1,
		  .samples = VK_SAMPLE_COUNT_1_BIT,
		  .tiling = VK_IMAGE_TILING_OPTIMAL },
		{ .pipelineStages = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		  .accessTypes = VK_ACCESS_SHADER_WRITE_BIT,
		  .startLayout = VK_IMAGE_LAYOUT_GENERAL,
		  .finishLayout = VK_IMAGE_LAYOUT_GENERAL,
		  .usageFlags = VK_IMAGE_USAGE_STORAGE_BIT,
		  .subresourceRange = lutSubresourceRange,
		  .writes = true,
		  .viewInfo = VImageResourceViewInfo{ .viewType = VK_IMAGE_VIEW_TYPE_2D,
											  .components = { .r = VK_COMPONENT_SWIZZLE_IDENTITY,
															  .g = VK_COMPONENT_SWIZZLE_IDENTITY,
															  .b = VK_COMPONENT_SWIZZLE_IDENTITY,
															  .a = VK_COMPONENT_SWIZZLE_IDENTITY },
											  .subresourceRange = lutSubresourceRange } });

	m_skyViewLUTHandle = context->declareTransientImage(
		this,
		{ .imageType = VK_IMAGE_TYPE_2D,
		  .format = VK_FORMAT_R32G32B32A32_SFLOAT,
		  .extent = { .width = 200, .height = 100, .depth = 1 },
		  .mipLevels = 1,
		  .arrayLayers = 1,
		  .samples = VK_SAMPLE_COUNT_1_BIT,
		  .tiling = VK_IMAGE_TILING_OPTIMAL },
		{ .pipelineStages = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		  .accessTypes = VK_ACCESS_SHADER_WRITE_BIT,
		  .startLayout = VK_IMAGE_LAYOUT_GENERAL,
		  .finishLayout = VK_IMAGE_LAYOUT_GENERAL,
		  .usageFlags = VK_IMAGE_USAGE_STORAGE_BIT,
		  .subresourceRange = lutSubresourceRange,
		  .writes = true,
		  .viewInfo = VImageResourceViewInfo{ .viewType = VK_IMAGE_VIEW_TYPE_2D,
											  .components = { .r = VK_COMPONENT_SWIZZLE_IDENTITY,
															  .g = VK_COMPONENT_SWIZZLE_IDENTITY,
															  .b = VK_COMPONENT_SWIZZLE_IDENTITY,
															  .a = VK_COMPONENT_SWIZZLE_IDENTITY },
											  .subresourceRange = lutSubresourceRange } });

	m_aerialPerspectiveLUTHandle = context->declareTransientImage(
		this,
		{ .imageType = VK_IMAGE_TYPE_3D,
		  .format = VK_FORMAT_R32G32B32A32_SFLOAT,
		  .extent = { .width = 32, .height = 32, .depth = 32 },
		  .mipLevels = 1,
		  .arrayLayers = 1,
		  .samples = VK_SAMPLE_COUNT_1_BIT,
		  .tiling = VK_IMAGE_TILING_OPTIMAL },
		{ .pipelineStages = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		  .accessTypes = VK_ACCESS_SHADER_WRITE_BIT,
		  .startLayout = VK_IMAGE_LAYOUT_GENERAL,
		  .finishLayout = VK_IMAGE_LAYOUT_GENERAL,
		  .usageFlags = VK_IMAGE_USAGE_STORAGE_BIT,
		  .subresourceRange = lutSubresourceRange,
		  .writes = true,
		  .viewInfo = VImageResourceViewInfo{ .viewType = VK_IMAGE_VIEW_TYPE_3D,
											  .components = { .r = VK_COMPONENT_SWIZZLE_IDENTITY,
															  .g = VK_COMPONENT_SWIZZLE_IDENTITY,
															  .b = VK_COMPONENT_SWIZZLE_IDENTITY,
															  .a = VK_COMPONENT_SWIZZLE_IDENTITY },
											  .subresourceRange = lutSubresourceRange } });

	m_multiscatterLUTHandle = context->declareTransientImage(
		this,
		{ .imageType = VK_IMAGE_TYPE_3D,
		  .format = VK_FORMAT_R32G32B32A32_SFLOAT,
		  .extent = { .width = 32, .height = 32, .depth = 32 },
		  .mipLevels = 1,
		  .arrayLayers = 1,
		  .samples = VK_SAMPLE_COUNT_1_BIT,
		  .tiling = VK_IMAGE_TILING_OPTIMAL },
		{ .pipelineStages = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		  .accessTypes = VK_ACCESS_SHADER_WRITE_BIT,
		  .startLayout = VK_IMAGE_LAYOUT_GENERAL,
		  .finishLayout = VK_IMAGE_LAYOUT_GENERAL,
		  .usageFlags = VK_IMAGE_USAGE_STORAGE_BIT,
		  .subresourceRange = lutSubresourceRange,
		  .writes = true,
		  .viewInfo = VImageResourceViewInfo{ .viewType = VK_IMAGE_VIEW_TYPE_3D,
											  .components = { .r = VK_COMPONENT_SWIZZLE_IDENTITY,
															  .g = VK_COMPONENT_SWIZZLE_IDENTITY,
															  .b = VK_COMPONENT_SWIZZLE_IDENTITY,
															  .a = VK_COMPONENT_SWIZZLE_IDENTITY },
											  .subresourceRange = lutSubresourceRange } });

	VkDescriptorSetLayoutBinding pipelineOutputBinding = { .binding = 0,
														   .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
														   .descriptorCount = 1,
														   .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT };
	VkDescriptorSetLayoutCreateInfo pipelineOutputLayoutCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = 1,
		.pBindings = &pipelineOutputBinding
	};
	verifyResult(vkCreateDescriptorSetLayout(context->gpuContext()->device(), &pipelineOutputLayoutCreateInfo, nullptr,
											 &m_pipelineOutputLayout));

	VkPushConstantRange range = { .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
								  .offset = 0,
								  .size = sizeof(TransmittanceComputeData) };

	VkPipelineLayoutCreateInfo transmittanceLayoutCreateInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
																 .setLayoutCount = 1,
																 .pSetLayouts = &m_pipelineOutputLayout,
																 .pushConstantRangeCount = 1,
																 .pPushConstantRanges = &range };
	verifyResult(vkCreatePipelineLayout(context->gpuContext()->device(), &transmittanceLayoutCreateInfo, nullptr,
										&m_transmittanceComputationPipelineLayout));

	VkShaderModule transmittanceComputeShaderModule;
	size_t shaderFileSize;
	void* data = readFile("./shaders/transmittance-comp.spv", &shaderFileSize);

	VkShaderModuleCreateInfo transmittanceModuleCreateInfo = { .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
															   .codeSize = shaderFileSize,
															   .pCode = reinterpret_cast<uint32_t*>(data) };
	verifyResult(vkCreateShaderModule(context->gpuContext()->device(), &transmittanceModuleCreateInfo, nullptr,
									  &transmittanceComputeShaderModule));

	VkComputePipelineCreateInfo transmittancePipelineCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		.stage = { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				   .stage = VK_SHADER_STAGE_COMPUTE_BIT,
				   .module = transmittanceComputeShaderModule,
				   .pName = "main" },
		.layout = m_transmittanceComputationPipelineLayout,
		.basePipelineIndex = -1
	};

	verifyResult(vkCreateComputePipelines(context->gpuContext()->device(), VK_NULL_HANDLE, 1,
										  &transmittancePipelineCreateInfo, nullptr,
										  &m_transmittanceComputationPipeline));

	VDescriptorSetAllocationInfo transmittanceSetAllocationInfo = {
		.typeInfos = { { .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .count = 1 } }, .layout = m_pipelineOutputLayout
	};
	VDescriptorSetAllocation transmittanceSetAllocation =
		context->descriptorSetAllocator()->allocateDescriptorSets({ transmittanceSetAllocationInfo })[0];
	m_transmittanceComputationOutputSet = transmittanceSetAllocation.set;

	vkDestroyShaderModule(context->gpuContext()->device(), transmittanceComputeShaderModule, nullptr);
}

void ScatteringLUTNode::initResources(VFramegraphContext* context) {
	VkImageView transmittanceImageView = context->imageView(this, m_transmittanceLUTHandle);

	VkDescriptorImageInfo transmittanceImageInfo = { .imageView = transmittanceImageView,
													 .imageLayout = VK_IMAGE_LAYOUT_GENERAL };
	VkWriteDescriptorSet transmittanceImageWrite = { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
													 .dstSet = m_transmittanceComputationOutputSet,
													 .dstBinding = 0,
													 .dstArrayElement = 0,
													 .descriptorCount = 1,
													 .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
													 .pImageInfo = &transmittanceImageInfo };
	vkUpdateDescriptorSets(context->gpuContext()->device(), 1, &transmittanceImageWrite, 0, nullptr);
}

void ScatteringLUTNode::recordCommands(VFramegraphContext* context, VkCommandBuffer targetCommandBuffer,
									   const VFramegraphNodeContext& nodeContext) {
	TransmittanceComputeData transmittanceData = {
		.lutSize = glm::ivec4(256, 64, 1, 1),
		.betaExtinctionZeroMie = glm::vec4(0.004440f),
		.betaExtinctionZeroRayleigh = glm::vec4(0.008612455256387713, 0.013011847505420919, 0.028268803129247174, 1.0f),
		.absorptionZeroOzone = glm::vec4(0.00065f, 0.001881f, 0.000085f, 1.0f),
		.heightScaleRayleigh = 8,
		.heightScaleMie = 1.2,
		.layerHeightOzone = 25.0f,
		.layer0ConstantFactorOzone = -2.0f / 3.0f,
		.layer1ConstantFactorOzone = 8.0f / 3.0f,
		.heightRangeOzone = 15.0f,
		.maxHeight = 100.0f,
		.groundRadius = 6360.0f
	};

	vkCmdBindPipeline(targetCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_transmittanceComputationPipeline);
	vkCmdBindDescriptorSets(targetCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
							m_transmittanceComputationPipelineLayout, 0, 1, &m_transmittanceComputationOutputSet, 0,
							nullptr);
	vkCmdPushConstants(targetCommandBuffer, m_transmittanceComputationPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0,
					   sizeof(TransmittanceComputeData), &transmittanceData);
	vkCmdDispatch(targetCommandBuffer, 32, 8, 1);
}

void ScatteringLUTNode::destroy(VFramegraphContext* context) {
	vkDestroyPipeline(context->gpuContext()->device(), m_transmittanceComputationPipeline, nullptr);
	vkDestroyPipelineLayout(context->gpuContext()->device(), m_transmittanceComputationPipelineLayout, nullptr);
	vkDestroyDescriptorSetLayout(context->gpuContext()->device(), m_pipelineOutputLayout, nullptr);
}
