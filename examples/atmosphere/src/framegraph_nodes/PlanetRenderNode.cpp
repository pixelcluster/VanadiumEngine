#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <framegraph_nodes/PlanetRenderNode.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <graphics/helper/ErrorHelper.hpp>
#include <helper/WholeFileReader.hpp>
#include <volk.h>

using namespace vanadium::graphics;

PlanetRenderNode::PlanetRenderNode() {}

void PlanetRenderNode::create(FramegraphContext* context) {
	FramegraphNodeImageUsage usage = {
		.subresourceAccesses = { {
			.accessingPipelineStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.access = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1 },
			.startLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.finishLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.writes = true
		} },
		.usageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.writes = true,
		.viewInfos = { {
			.viewType = VK_IMAGE_VIEW_TYPE_2D, 
			.components = {
				.r = VK_COMPONENT_SWIZZLE_IDENTITY,
				.g = VK_COMPONENT_SWIZZLE_IDENTITY,
				.b = VK_COMPONENT_SWIZZLE_IDENTITY,
				.a = VK_COMPONENT_SWIZZLE_IDENTITY,
			},
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1
			}
		} } 
	};
	context->declareReferencedSwapchainImage(this, usage);

	VkAttachmentDescription description = { .format = context->renderContext().targetSurface->properties().format,
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

	m_pipelineID = context->renderContext().pipelineLibrary->findGraphicsPipeline("Planet Drawing");
	context->renderContext().pipelineLibrary->createForPass(m_passSignature, m_renderPass, { m_pipelineID });
}

void PlanetRenderNode::recordCommands(FramegraphContext* context, VkCommandBuffer targetCommandBuffer,
									  const FramegraphNodeContext& nodeContext) {
	VkClearValue value = { .color = { .float32 = { 0.0f, 0.0f, 0.0f } } };

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
	VkBuffer vertexBuffer = context->renderContext().resourceAllocator->nativeBufferHandle(m_vertexData);
	VkBuffer indexBuffer = context->renderContext().resourceAllocator->nativeBufferHandle(m_indexData);

	vkCmdBindVertexBuffers(targetCommandBuffer, 0, 1, &vertexBuffer, &offset);
	vkCmdBindIndexBuffer(targetCommandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

	VkDescriptorSet sets[2] = { m_uboSet, m_texSet };

	vkCmdBindDescriptorSets(targetCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
							context->renderContext().pipelineLibrary->graphicsPipelineLayout(m_pipelineID), 0, 2, sets,
							0, nullptr);

	vkCmdDrawIndexed(targetCommandBuffer, m_indexCount, 1, 0, 0, 0);

	vkCmdEndRenderPass(targetCommandBuffer);
}

void PlanetRenderNode::setupObjects(BufferResourceHandle vertexDataBuffer, BufferResourceHandle indexDataBuffer,
									VkDescriptorSetLayout sceneDataLayout, VkDescriptorSet sceneDataSet,
									VkDescriptorSetLayout texSetLayout, VkDescriptorSet texSet, uint32_t indexCount) {
	m_vertexData = vertexDataBuffer;
	m_indexData = indexDataBuffer;
	m_texSetLayout = texSetLayout;
	m_texSet = texSet;
	m_uboSetLayout = sceneDataLayout;
	m_uboSet = sceneDataSet;
	m_indexCount = indexCount;
	m_name = "Planet rendering";
}

void PlanetRenderNode::recreateSwapchainResources(FramegraphContext* context, uint32_t width, uint32_t height) {
	for (auto& framebuffer : m_framebuffers) {
		vkDestroyFramebuffer(context->renderContext().deviceContext->device(), framebuffer, nullptr);
	}
	m_framebuffers.resize(context->renderContext().targetSurface->currentImageCount());

	uint32_t index = 0;
	for (auto& framebuffer : m_framebuffers) {
		VkImageView targetImageView = context->renderContext().targetSurface->targetView(index, {
			.viewType = VK_IMAGE_VIEW_TYPE_2D, 
			.components = {
				.r = VK_COMPONENT_SWIZZLE_IDENTITY,
				.g = VK_COMPONENT_SWIZZLE_IDENTITY,
				.b = VK_COMPONENT_SWIZZLE_IDENTITY,
				.a = VK_COMPONENT_SWIZZLE_IDENTITY,
			},
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1
			}
		});
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

void PlanetRenderNode::destroy(FramegraphContext* context) {
	for (auto& framebuffer : m_framebuffers) {
		vkDestroyFramebuffer(context->renderContext().deviceContext->device(), framebuffer, nullptr);
	}

	vkDestroyRenderPass(context->renderContext().deviceContext->device(), m_renderPass, nullptr);
}
