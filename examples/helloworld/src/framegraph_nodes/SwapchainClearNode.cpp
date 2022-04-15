#include <framegraph_nodes/SwapchainClearNode.hpp>
#include <volk.h>

using namespace vanadium::graphics;

void SwapchainClearNode::setupResources(FramegraphContext* context) {
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
