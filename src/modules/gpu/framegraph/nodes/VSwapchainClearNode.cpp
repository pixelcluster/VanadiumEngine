#include <modules/gpu/framegraph/nodes/VSwapchainClearNode.hpp>
#include <volk.h>

VSwapchainClearNode::VSwapchainClearNode() {}

void VSwapchainClearNode::setupResources(VFramegraphContext* context) {
	context->declareReferencedImage(this, "Swapchain image",
									{
										.pipelineStages = VK_PIPELINE_STAGE_TRANSFER_BIT,
										.accessTypes = VK_ACCESS_TRANSFER_WRITE_BIT,
										.startLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
										.finishLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
										.usageFlags = VK_IMAGE_USAGE_TRANSFER_DST_BIT
									});
}

void VSwapchainClearNode::recordCommands(VFramegraphContext* context, VkCommandBuffer targetCommandBuffer) {
	VkClearColorValue clearValue = { .float32 = { 0.2f, 0.2f, 0.2f, 1.0f } };

	VkImageSubresourceRange resourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
											  .baseMipLevel = 0,
											  .levelCount = 1,
											  .baseArrayLayer = 0,
											  .layerCount = 1 };

	vkCmdClearColorImage(targetCommandBuffer, context->nativeImageHandle("Swapchain image"),
						 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearValue, 1, &resourceRange);
}
