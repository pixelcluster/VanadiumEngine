#include <modules/gpu/framegraph/nodes/VSwapchainClearNode.hpp>
#include <volk.h>

VSwapchainClearNode::VSwapchainClearNode() {}

void VSwapchainClearNode::setupResources(VFramegraphContext* context) {

}

void VSwapchainClearNode::recordCommands(VFramegraphContext* context, VkCommandBuffer targetCommandBuffer) {
	VkClearColorValue clearValue = { .float32 = { 0.2f, 0.2f, 0.2f, 1.0f } };

	VkImageSubresourceRange resourceRange = { 
		.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.baseMipLevel = 0,
		.levelCount = 1,
		.baseArrayLayer = 0,
		.layerCount = 1
	};

	vkCmdClearColorImage(targetCommandBuffer, context->nativeImageHandle("Swapchain image"),
						 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearValue, 1, &resourceRange);
}
