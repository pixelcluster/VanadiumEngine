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
#include <framegraph_nodes/ScatteringLUTNode.hpp>
#include <graphics/helper/DescriptorSetAllocateInfoGenerator.hpp>
#include <graphics/helper/ErrorHelper.hpp>
#include <volk.h>

using namespace vanadium::graphics;

void ScatteringLUTNode::create(FramegraphContext* context) {
	m_transmittanceComputationID =
		context->renderContext().pipelineLibrary->findComputePipeline("Transmittance Precomputation");
	m_skyViewComputationID = context->renderContext().pipelineLibrary->findComputePipeline("Sky-view Precomputation");

	DescriptorSetAllocation transmittanceSetAllocation =
		context->renderContext().descriptorSetAllocator->allocateDescriptorSets(allocationInfosForPipeline(
			context->renderContext().pipelineLibrary, PipelineType::Compute, m_transmittanceComputationID))[0];
	m_transmittanceComputationOutputSet = transmittanceSetAllocation.set;

	std::vector<DescriptorSetAllocationInfo> allocationInfos = allocationInfosForPipeline(
		context->renderContext().pipelineLibrary, PipelineType::Compute, m_skyViewComputationID);

	std::vector<DescriptorSetAllocation> skyViewSetAllocations =
		context->renderContext().descriptorSetAllocator->allocateDescriptorSets(
			std::vector<DescriptorSetAllocationInfo>(frameInFlightCount, allocationInfos[1]));
	for (unsigned int i = 0; i < frameInFlightCount; ++i) {
		m_skyViewComputationInputSets[i] = skyViewSetAllocations[i].set;
	}

	m_skyViewComputationOutputSet =
		context->renderContext().descriptorSetAllocator->allocateDescriptorSets({ allocationInfos[0] })[0].set;

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
											 .accessingPipelineStages = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
											 .access = VK_ACCESS_SHADER_WRITE_BIT,
											 .subresourceRange = lutSubresourceRange,
											 .startLayout = VK_IMAGE_LAYOUT_GENERAL,
											 .finishLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
											 .writes = true,
										 } },

										 .usageFlags = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
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
		{ .subresourceAccesses = { { .accessingPipelineStages = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
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
		{ .subresourceAccesses = { { .accessingPipelineStages = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
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
		{ .subresourceAccesses = { { .accessingPipelineStages = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
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

	m_skyViewInputBufferHandle = context->renderContext().resourceAllocator->createPerFrameBuffer(
		{ .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		  .size = sizeof(SkyViewComputeData),
		  .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT },
		{ .hostVisible = true }, { .deviceLocal = true }, true);
}

void ScatteringLUTNode::afterResourceInit(FramegraphContext* context) {
	VkImageView transmittanceImageView = context->imageView(this, m_transmittanceLUTHandle, 0);
	VkImageView skyViewImageView = context->imageView(this, m_skyViewLUTHandle, 0);

	VkDescriptorImageInfo transmittanceImageInfo = { .imageView = transmittanceImageView,
													 .imageLayout = VK_IMAGE_LAYOUT_GENERAL };
	VkDescriptorImageInfo skyViewImageInfo = { .imageView = skyViewImageView, .imageLayout = VK_IMAGE_LAYOUT_GENERAL };
	VkWriteDescriptorSet imageWriteSets[2] = {
		{ .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		  .dstSet = m_transmittanceComputationOutputSet,
		  .dstBinding = 0,
		  .dstArrayElement = 0,
		  .descriptorCount = 1,
		  .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		  .pImageInfo = &transmittanceImageInfo },
		{ .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		  .dstSet = m_skyViewComputationOutputSet,
		  .dstBinding = 0,
		  .dstArrayElement = 0,
		  .descriptorCount = 1,
		  .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		  .pImageInfo = &skyViewImageInfo },
	};
	vkUpdateDescriptorSets(context->renderContext().deviceContext->device(), 2, imageWriteSets, 0, nullptr);
}

void ScatteringLUTNode::recordCommands(FramegraphContext* context, VkCommandBuffer targetCommandBuffer,
									   const FramegraphNodeContext& nodeContext) {
	TransmittanceComputeData transmittanceData = {
		.lutSize = glm::ivec4(256, 64, 1, 1),
		.betaExtinctionZeroMie = glm::vec4(0.003996f),
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

	SkyViewComputeData skyViewData = { .commonData = transmittanceData,
									   .currentHeight = 0.3f,
									   .rayleighScattering = transmittanceData.betaExtinctionZeroRayleigh,
									   .mieScattering = 0.003996f,
									   .sunSphere =
										   Sphere{ .center = glm::vec2(glm::radians(10.0f), glm::radians(69.0f)),
												   .radiusSquared = glm::radians(10.0f) },
									   .sunLuminance = glm::vec4(1.0f) };
	skyViewData.commonData.lutSize = glm::ivec4(200, 100, 1, 1);
	std::memcpy(context->renderContext().resourceAllocator->mappedBufferData(m_skyViewInputBufferHandle), &skyViewData,
				sizeof(SkyViewComputeData));

	VkDescriptorBufferInfo skyViewInputBufferInfo = {
		.buffer = context->renderContext().resourceAllocator->nativeBufferHandle(m_skyViewInputBufferHandle),
		.offset = 0,
		.range = VK_WHOLE_SIZE
	};
	VkImageView transmittanceImageView = context->imageView(this, m_transmittanceLUTHandle, 0);
	VkDescriptorImageInfo skyViewTransmittanceLUTInfo = { .imageView = transmittanceImageView,
														  .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
	VkWriteDescriptorSet skyViewInputWrites[2] = { { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
													 .dstSet = m_skyViewComputationInputSets[nodeContext.frameIndex],
													 .dstBinding = 1,
													 .dstArrayElement = 0,
													 .descriptorCount = 1,
													 .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
													 .pBufferInfo = &skyViewInputBufferInfo },
												   { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
													 .dstSet = m_skyViewComputationInputSets[nodeContext.frameIndex],
													 .dstBinding = 0,
													 .dstArrayElement = 0,
													 .descriptorCount = 1,
													 .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
													 .pImageInfo = &skyViewTransmittanceLUTInfo } };
	vkUpdateDescriptorSets(context->renderContext().deviceContext->device(), 2, skyViewInputWrites, 0, nullptr);

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

	VkImageMemoryBarrier transmittanceLUTTransition = { .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
														.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
														.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
														.oldLayout = VK_IMAGE_LAYOUT_GENERAL,
														.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
														.image = context->nativeImageHandle(m_transmittanceLUTHandle),
														.subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
																			  .baseMipLevel = 0,
																			  .levelCount = 1,
																			  .baseArrayLayer = 0,
																			  .layerCount = 1 } };
	vkCmdPipelineBarrier(targetCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
						 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
						 &transmittanceLUTTransition);
	vkCmdBindPipeline(targetCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
					  context->renderContext().pipelineLibrary->computePipeline(m_skyViewComputationID));
	VkDescriptorSet skyViewInputOutputSets[2] = { m_skyViewComputationOutputSet,
												  m_skyViewComputationInputSets[nodeContext.frameIndex] };
	vkCmdBindDescriptorSets(targetCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
							context->renderContext().pipelineLibrary->computePipelineLayout(m_skyViewComputationID), 0,
							2, skyViewInputOutputSets, 0, nullptr);
	vkCmdDispatch(targetCommandBuffer, 25, 13, 1);
}

void ScatteringLUTNode::destroy(FramegraphContext* context) {}
