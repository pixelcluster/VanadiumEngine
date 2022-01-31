#include <modules/gpu/framegraph/nodes/VHelloTriangleNode.hpp>

void VHelloTriangleNode::setupResources(VFramegraphContext* context) {
	VkAttachmentDescription description = {
		.format = VK_FORMAT_B8G8R8A8_SRGB,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	};

	VkAttachmentReference reference = { .attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

	VkSubpassDescription subpassDescription = { .colorAttachmentCount = 1, .pColorAttachments = &reference };
}

void VHelloTriangleNode::recordCommands(VFramegraphContext* context, VkCommandBuffer targetCommandBuffer) {}

void VHelloTriangleNode::handleWindowResize(VFramegraphContext* context) {}
