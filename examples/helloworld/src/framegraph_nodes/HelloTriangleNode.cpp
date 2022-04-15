#include <framegraph_nodes/HelloTriangleNode.hpp>
#include <graphics/helper/ErrorHelper.hpp>
#include <volk.h>

#include <helper/WholeFileReader.hpp>

using namespace vanadium::graphics;

HelloTriangleNode::HelloTriangleNode(VertexBufferUpdater* updater) : m_bufferUpdater(updater) {
	m_name = "Triangle drawing";
	m_swapchainViewInfo = { .viewType = VK_IMAGE_VIEW_TYPE_2D,
							.subresourceRange = {
								.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 } };
}

void HelloTriangleNode::setupResources(FramegraphContext* context) {
	m_pipelineID = context->renderContext().pipelineLibrary->findGraphicsPipeline("Hello Triangle");

	VkAttachmentDescription description = { .format = VK_FORMAT_B8G8R8A8_SRGB,
											.samples = VK_SAMPLE_COUNT_1_BIT,
											.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
											.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
											.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
											.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
											.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
											.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

	VkAttachmentReference reference = { .attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

	VkSubpassDescription subpassDescription = { .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
												.colorAttachmentCount = 1,
												.pColorAttachments = &reference };

	VkRenderPassCreateInfo renderPassCreateInfo = { .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
													.attachmentCount = 1,
													.pAttachments = &description,
													.subpassCount = 1,
													.pSubpasses = &subpassDescription };
	verifyResult(vkCreateRenderPass(context->renderContext().deviceContext->device(), &renderPassCreateInfo, nullptr,
									&m_renderPass));

	m_passSignature = { .subpassSignatures = { { .outputAttachments = { { .isUsed = true,
																		  .format = VK_FORMAT_B8G8R8A8_SRGB,
																		  .sampleCount = VK_SAMPLE_COUNT_1_BIT } } } },
						.attachmentDescriptionSignatures = { { .isUsed = true,
															   .format = VK_FORMAT_B8G8R8A8_SRGB,
															   .sampleCount = VK_SAMPLE_COUNT_1_BIT } } };

	context->renderContext().pipelineLibrary->createForPass(m_passSignature, m_renderPass, { m_pipelineID });

	FramegraphNodeImageUsage usage = {
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
					.startLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					.finishLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					.writes = true
				}
			},
			.usageFlags = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			.writes = true,
			.viewInfos = { m_swapchainViewInfo }
		};

	context->declareReferencedSwapchainImage(this, usage);
	context->declareImportedBuffer(
		this, m_bufferUpdater->vertexBufferHandle(context->renderContext()),
		{
			.subresourceAccesses = { { .accessingPipelineStages = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
									   .access = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
									   .offset = 0,
									   .size = VK_WHOLE_SIZE,
									   .writes = false } },
			.writes = false,
			.usageFlags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		});
}

void HelloTriangleNode::recordCommands(FramegraphContext* context, VkCommandBuffer targetCommandBuffer,
									   const FramegraphNodeContext& nodeContext) {
	VkClearValue value = { .color = { .float32 = { 0.2f, 0.2f, 0.2f } } };

	VkRenderPassBeginInfo beginInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = m_renderPass,
		.framebuffer = m_framebuffers[context->renderContext().targetSurface->currentTargetIndex()],
		.renderArea = { .extent = { m_width, m_height } },
		.clearValueCount = 1,
		.pClearValues = &value
	};
	vkCmdBeginRenderPass(targetCommandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport = { .width = static_cast<float>(m_width),
							.height = static_cast<float>(m_height),
							.maxDepth = 1.0f };

	vkCmdBindPipeline(targetCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
					  context->renderContext().pipelineLibrary->graphicsPipeline(m_pipelineID, m_passSignature));

	vkCmdSetViewport(targetCommandBuffer, 0, 1, &viewport);

	VkDeviceSize offset = 0;
	VkBuffer nativeBuffer = context->renderContext().resourceAllocator->nativeBufferHandle(
		m_bufferUpdater->vertexBufferHandle(context->renderContext()));
	vkCmdBindVertexBuffers(targetCommandBuffer, 0, 1, &nativeBuffer, &offset);

	vkCmdDraw(targetCommandBuffer, 3, 1, 0, 0);

	vkCmdEndRenderPass(targetCommandBuffer);
}

void HelloTriangleNode::handleWindowResize(FramegraphContext* context, uint32_t width, uint32_t height) {
	for (auto& framebuffer : m_framebuffers) {
		vkDestroyFramebuffer(context->renderContext().deviceContext->device(), framebuffer, nullptr);
	}
	m_framebuffers.resize(context->renderContext().targetSurface->currentImageCount());

	uint32_t index = 0;
	for (auto& framebuffer : m_framebuffers) {
		VkImageView targetImageView = context->renderContext().targetSurface->targetView(index, m_swapchainViewInfo);
		VkFramebufferCreateInfo framebufferCreateInfo = { .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
														  .renderPass = m_renderPass,
														  .attachmentCount = 1,
														  .pAttachments = &targetImageView,
														  .width = width,
														  .height = height,
														  .layers = 1 };
		verifyResult(vkCreateFramebuffer(context->renderContext().deviceContext->device(), &framebufferCreateInfo,
										 nullptr, &framebuffer));
		++index;
	}

	m_width = width;
	m_height = height;
}

void HelloTriangleNode::destroy(FramegraphContext* context) {
	for (auto& framebuffer : m_framebuffers) {
		vkDestroyFramebuffer(context->renderContext().deviceContext->device(), framebuffer, nullptr);
	}

	vkDestroyRenderPass(context->renderContext().deviceContext->device(), m_renderPass, nullptr);
}