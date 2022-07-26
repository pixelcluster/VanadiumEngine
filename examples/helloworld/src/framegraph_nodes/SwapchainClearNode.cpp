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
#include <framegraph_nodes/SwapchainClearNode.hpp>
#include <volk.h>

using namespace vanadium::graphics;

void SwapchainClearNode::create(FramegraphContext* context) {
	context->declareReferencedSwapchainImage(this, {
			.subresourceAccesses = {
				NodeImageSubresourceAccess {
					.accessingPipelineStages = VK_PIPELINE_STAGE_TRANSFER_BIT,
					.access = VK_ACCESS_TRANSFER_WRITE_BIT,
					.subresourceRange = {
						.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
    					.baseMipLevel = 0,
    					.levelCount = 1,
    					.baseArrayLayer = 0,
    					.layerCount = 1,
					},
					.startLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					.finishLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					.writes = true
				}
			},
			.usageFlags = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			.writes = true
		});
}

void SwapchainClearNode::recordCommands(FramegraphContext* context, VkCommandBuffer targetCommandBuffer,
										const FramegraphNodeContext& nodeContext) {
	VkClearColorValue clearValue = { .float32 = { 0.2f, 0.2f, 0.2f, 1.0f } };

	VkImageSubresourceRange resourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
											  .baseMipLevel = 0,
											  .levelCount = 1,
											  .baseArrayLayer = 0,
											  .layerCount = 1 };

	vkCmdClearColorImage(targetCommandBuffer, nodeContext.targetSurface->currentTargetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						 &clearValue, 1, &resourceRange);
}
