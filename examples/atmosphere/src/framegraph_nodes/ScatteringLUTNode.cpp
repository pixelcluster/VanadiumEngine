#include <framegraph_nodes/ScatteringLUTNode.hpp>
#include <graphics/helper/DescriptorSetAllocateInfoGenerator.hpp>
#include <graphics/helper/ErrorHelper.hpp>
#include <helper/WholeFileReader.hpp>
#include <volk.h>

using namespace vanadium::graphics;

void ScatteringLUTNode::create(FramegraphContext* context) {
	m_transmittanceComputationID =
		context->renderContext().pipelineLibrary->findComputePipeline("Transmittance Precomputation");

	DescriptorSetAllocation transmittanceSetAllocation =
		context->renderContext().descriptorSetAllocator->allocateDescriptorSets(allocationInfosForPipeline(
			context->renderContext().pipelineLibrary, PipelineType::Compute, m_transmittanceComputationID))[0];
	m_transmittanceComputationOutputSet = transmittanceSetAllocation.set;
}

void ScatteringLUTNode::setupResources(vanadium::graphics::FramegraphContext* context) {
	VkImageSubresourceRange lutSubresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
													.baseMipLevel = 0,
													.levelCount = 1,
													.baseArrayLayer = 0,
													.layerCount = 1 };
	m_transmittanceLUTHandle =
		context->declareTransientImage(this,
									   { .imageType = VK_IMAGE_TYPE_2D,
										 .format = VK_FORMAT_R32G32B32A32_SFLOAT,
										 .extent = { .width = 256, .height = 64, .depth = 1 },
										 .mipLevels = 1,
										 .arrayLayers = 1,
										 .samples = VK_SAMPLE_COUNT_1_BIT,
										 .tiling = VK_IMAGE_TILING_OPTIMAL },
									   { .subresourceAccesses = { {
											 .accessingPipelineStages = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
											 .access = VK_ACCESS_SHADER_WRITE_BIT,
											 .subresourceRange = lutSubresourceRange,
											 .startLayout = VK_IMAGE_LAYOUT_GENERAL,
											 .finishLayout = VK_IMAGE_LAYOUT_GENERAL,
											 .writes = true,
										 } },

										 .usageFlags = VK_IMAGE_USAGE_STORAGE_BIT,
										 .writes = true,
										 .viewInfos = { { .viewType = VK_IMAGE_VIEW_TYPE_2D,
														  .components = { .r = VK_COMPONENT_SWIZZLE_IDENTITY,
																		  .g = VK_COMPONENT_SWIZZLE_IDENTITY,
																		  .b = VK_COMPONENT_SWIZZLE_IDENTITY,
																		  .a = VK_COMPONENT_SWIZZLE_IDENTITY },
														  .subresourceRange = lutSubresourceRange } } });

	m_skyViewLUTHandle = context->declareTransientImage(
		this,
		{ .imageType = VK_IMAGE_TYPE_2D,
		  .format = VK_FORMAT_R32G32B32A32_SFLOAT,
		  .extent = { .width = 200, .height = 100, .depth = 1 },
		  .mipLevels = 1,
		  .arrayLayers = 1,
		  .samples = VK_SAMPLE_COUNT_1_BIT,
		  .tiling = VK_IMAGE_TILING_OPTIMAL },
		{ .subresourceAccesses = { { .accessingPipelineStages = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
									 .access = VK_ACCESS_SHADER_WRITE_BIT,
									 .subresourceRange = lutSubresourceRange,
									 .startLayout = VK_IMAGE_LAYOUT_GENERAL,
									 .finishLayout = VK_IMAGE_LAYOUT_GENERAL,
									 .writes = true } },
		  .usageFlags = VK_IMAGE_USAGE_STORAGE_BIT,
		  .writes = true,
		  .viewInfos = { { .viewType = VK_IMAGE_VIEW_TYPE_2D,
						   .components = { .r = VK_COMPONENT_SWIZZLE_IDENTITY,
										   .g = VK_COMPONENT_SWIZZLE_IDENTITY,
										   .b = VK_COMPONENT_SWIZZLE_IDENTITY,
										   .a = VK_COMPONENT_SWIZZLE_IDENTITY },
						   .subresourceRange = lutSubresourceRange } } });

	m_aerialPerspectiveLUTHandle = context->declareTransientImage(
		this,
		{ .imageType = VK_IMAGE_TYPE_3D,
		  .format = VK_FORMAT_R32G32B32A32_SFLOAT,
		  .extent = { .width = 32, .height = 32, .depth = 32 },
		  .mipLevels = 1,
		  .arrayLayers = 1,
		  .samples = VK_SAMPLE_COUNT_1_BIT,
		  .tiling = VK_IMAGE_TILING_OPTIMAL },
		{ .subresourceAccesses = { { .accessingPipelineStages = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
									 .access = VK_ACCESS_SHADER_WRITE_BIT,
									 .subresourceRange = lutSubresourceRange,
									 .startLayout = VK_IMAGE_LAYOUT_GENERAL,
									 .finishLayout = VK_IMAGE_LAYOUT_GENERAL,
									 .writes = true } },
		  .usageFlags = VK_IMAGE_USAGE_STORAGE_BIT,
		  .writes = true,
		  .viewInfos = { { .viewType = VK_IMAGE_VIEW_TYPE_3D,
						   .components = { .r = VK_COMPONENT_SWIZZLE_IDENTITY,
										   .g = VK_COMPONENT_SWIZZLE_IDENTITY,
										   .b = VK_COMPONENT_SWIZZLE_IDENTITY,
										   .a = VK_COMPONENT_SWIZZLE_IDENTITY },
						   .subresourceRange = lutSubresourceRange } } });

	m_multiscatterLUTHandle = context->declareTransientImage(
		this,
		{ .imageType = VK_IMAGE_TYPE_3D,
		  .format = VK_FORMAT_R32G32B32A32_SFLOAT,
		  .extent = { .width = 32, .height = 32, .depth = 32 },
		  .mipLevels = 1,
		  .arrayLayers = 1,
		  .samples = VK_SAMPLE_COUNT_1_BIT,
		  .tiling = VK_IMAGE_TILING_OPTIMAL },
		{ .subresourceAccesses = { { .accessingPipelineStages = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
									 .access = VK_ACCESS_SHADER_WRITE_BIT,
									 .subresourceRange = lutSubresourceRange,
									 .startLayout = VK_IMAGE_LAYOUT_GENERAL,
									 .finishLayout = VK_IMAGE_LAYOUT_GENERAL,
									 .writes = true } },
		  .usageFlags = VK_IMAGE_USAGE_STORAGE_BIT,
		  .writes = true,
		  .viewInfos = { { .viewType = VK_IMAGE_VIEW_TYPE_3D,
						   .components = { .r = VK_COMPONENT_SWIZZLE_IDENTITY,
										   .g = VK_COMPONENT_SWIZZLE_IDENTITY,
										   .b = VK_COMPONENT_SWIZZLE_IDENTITY,
										   .a = VK_COMPONENT_SWIZZLE_IDENTITY },
						   .subresourceRange = lutSubresourceRange } } });
}

void ScatteringLUTNode::initResources(FramegraphContext* context) {
	VkImageView transmittanceImageView = context->imageView(this, m_transmittanceLUTHandle, 0);

	VkDescriptorImageInfo transmittanceImageInfo = { .imageView = transmittanceImageView,
													 .imageLayout = VK_IMAGE_LAYOUT_GENERAL };
	VkWriteDescriptorSet transmittanceImageWrite = { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
													 .dstSet = m_transmittanceComputationOutputSet,
													 .dstBinding = 0,
													 .dstArrayElement = 0,
													 .descriptorCount = 1,
													 .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
													 .pImageInfo = &transmittanceImageInfo };
	vkUpdateDescriptorSets(context->renderContext().deviceContext->device(), 1, &transmittanceImageWrite, 0, nullptr);
}

void ScatteringLUTNode::recordCommands(FramegraphContext* context, VkCommandBuffer targetCommandBuffer,
									   const FramegraphNodeContext& nodeContext) {
	TransmittanceComputeData transmittanceData = {
		.lutSize = glm::ivec4(256, 64, 1, 1),
		.betaExtinctionZeroMie = glm::vec4(0.004440f),
		.betaExtinctionZeroRayleigh = glm::vec4(0.008612455256387713, 0.013011847505420919, 0.028268803129247174, 1.0f),
		.absorptionZeroOzone = glm::vec4(0.002366656826034617f, 0.001620425091578396f, 0.00010373049232375621f, 1.0f),
		.heightScaleRayleigh = 8,
		.heightScaleMie = 1.2,
		.layerHeightOzone = 25.0f,
		.layer0ConstantFactorOzone = -2.0f / 3.0f,
		.layer1ConstantFactorOzone = 8.0f / 3.0f,
		.heightRangeOzone = 15.0f,
		.maxHeight = 100.0f,
		.groundRadius = 6360.0f
	};

	vkCmdBindPipeline(targetCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
					  context->renderContext().pipelineLibrary->computePipeline(m_transmittanceComputationID));
	vkCmdBindDescriptorSets(
		targetCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
		context->renderContext().pipelineLibrary->computePipelineLayout(m_transmittanceComputationID), 0, 1,
		&m_transmittanceComputationOutputSet, 0, nullptr);
	vkCmdPushConstants(targetCommandBuffer,
					   context->renderContext().pipelineLibrary->computePipelineLayout(m_transmittanceComputationID),
					   VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(TransmittanceComputeData), &transmittanceData);
	vkCmdDispatch(targetCommandBuffer, 32, 8, 1);
}

void ScatteringLUTNode::destroy(FramegraphContext* context) {}
