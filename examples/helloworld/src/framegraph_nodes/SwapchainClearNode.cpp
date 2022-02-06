#include <framegraph_nodes/SwapchainClearNode.hpp>
#include <volk.h>

void SwapchainClearNode::setupResources(VFramegraphContext* context) {
	context->declareReferencedSwapchainImage(this, { .pipelineStages = VK_PIPELINE_STAGE_TRANSFER_BIT,
													 .accessTypes = VK_ACCESS_TRANSFER_WRITE_BIT,
													 .startLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
													 .finishLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
													 .usageFlags = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
													 .subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
																		   .baseMipLevel = 0,
																		   .levelCount = 1,
																		   .baseArrayLayer = 0,
																		   .layerCount = 1 },
													 .writes = true });
}

void SwapchainClearNode::recordCommands(VFramegraphContext* context, VkCommandBuffer targetCommandBuffer,
										const VFramegraphNodeContext& nodeContext) {
	VkClearColorValue clearValue = { .float32 = { 0.2f, 0.2f, 0.2f, 1.0f } };

	VkImageSubresourceRange resourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
											  .baseMipLevel = 0,
											  .levelCount = 1,
											  .baseArrayLayer = 0,
											  .layerCount = 1 };

	vkCmdClearColorImage(targetCommandBuffer, nodeContext.swapchainImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						 &clearValue, 1, &resourceRange);
}
