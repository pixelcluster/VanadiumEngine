#include <graphics/helper/DebugHelper.hpp>
#include <graphics/helper/ErrorHelper.hpp>
#include <ui/UIRendererNode.hpp>
#include <volk.h>

namespace vanadium::ui {
	void UIRendererNode::create(graphics::FramegraphContext* context) {
		m_framegraphContext = context;
		m_attachmentResourceViewInfo = {
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 }
		};
	}

	void UIRendererNode::setupResources(graphics::FramegraphContext* context) {
		context->declareReferencedSwapchainImage(this, {
			.subresourceAccesses = {
				graphics::NodeImageSubresourceAccess {
					.accessingPipelineStages = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
					.access = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT,
					.subresourceRange = {
						.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
    					.baseMipLevel = 0,
    					.levelCount = 1,
    					.baseArrayLayer = 0,
    					.layerCount = 1,
					},
					.startLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					.finishLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
					.writes = true
				}
			},
			.usageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			.writes = true,
			.viewInfos = { m_attachmentResourceViewInfo }
		});
		m_depthBufferHandle = context->declareTransientImage(
			this,
			 { .imageType = VK_IMAGE_TYPE_2D,
			  .format = VK_FORMAT_D32_SFLOAT,
			  .extent = { .depth = 1 },
			  .mipLevels = 1,
			  .arrayLayers = 1,
			  .samples = VK_SAMPLE_COUNT_1_BIT,
			  .tiling = VK_IMAGE_TILING_OPTIMAL,
			  .useTargetImageExtent = true }, {
			.subresourceAccesses = { {
					.accessingPipelineStages = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
					.access = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
					.subresourceRange = {
						.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
   						.baseMipLevel = 0,
   						.levelCount = 1,
   						.baseArrayLayer = 0,
   						.layerCount = 1,
					},
					.startLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
					.finishLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
					.writes = true
				}
			},
			.usageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			.writes = true,
			.viewInfos = { 
				{
					.viewType = VK_IMAGE_VIEW_TYPE_2D,
					.subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT, .levelCount = 1, .layerCount = 1 }
				} } 
		});
	} // namespace vanadium::ui

	void UIRendererNode::initResources(graphics::FramegraphContext* context) {
		VkAttachmentDescription attachmentDescriptions[2] = {
			{ .format = m_renderContext.targetSurface->properties().format,
			  .samples = VK_SAMPLE_COUNT_1_BIT,
			  .loadOp =
				  m_backgroundClearColor != Vector4(0.f) ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
			  .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			  .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
			  .stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE,
			  .initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			  .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR },
			{ .format = VK_FORMAT_D32_SFLOAT,
			  .samples = VK_SAMPLE_COUNT_1_BIT,
			  .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
			  .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			  .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
			  .stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE,
			  .initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			  .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL }
		};
		VkAttachmentReference targetAttachmentReference = { .attachment = 0,
															.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
		VkAttachmentReference depthAttachmentReference = { .attachment = 1,
														   .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
		VkSubpassDescription subpassDescription = { .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
													.colorAttachmentCount = 1,
													.pColorAttachments = &targetAttachmentReference,
													.pDepthStencilAttachment = &depthAttachmentReference };
		VkRenderPassCreateInfo createInfo = { .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
											  .attachmentCount = 2,
											  .pAttachments = attachmentDescriptions,
											  .subpassCount = 1,
											  .pSubpasses = &subpassDescription };
		verifyResult(
			vkCreateRenderPass(m_renderContext.deviceContext->device(), &createInfo, nullptr, &m_uiRenderPass));
		m_uiPassSignature = graphics::RenderPassSignature{
			.subpassSignatures = { { .outputAttachments = { { .isUsed = true,
															  .format =
																  m_renderContext.targetSurface->properties().format,
															  .sampleCount = VK_SAMPLE_COUNT_1_BIT } },
									 .depthStencilAttachment = { .isUsed = true,
																 .format = VK_FORMAT_D32_SFLOAT,
																 .sampleCount = VK_SAMPLE_COUNT_1_BIT } } },
			.attachmentDescriptionSignatures = { { .isUsed = true,
												   .format = m_renderContext.targetSurface->properties().format,
												   .sampleCount = VK_SAMPLE_COUNT_1_BIT },
												 { .isUsed = true,
												   .format = VK_FORMAT_D32_SFLOAT,
												   .sampleCount = VK_SAMPLE_COUNT_1_BIT } }
		};
	}

	void UIRendererNode::recordCommands(graphics::FramegraphContext* context, VkCommandBuffer targetCommandBuffer,
										const graphics::FramegraphNodeContext& nodeContext) {
		VkClearValue clearValue = { .color = { .float32 = { m_backgroundClearColor.r, m_backgroundClearColor.g,
															m_backgroundClearColor.b, m_backgroundClearColor.a } } };

		VkRenderPassBeginInfo beginInfo = {
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.renderPass = m_uiRenderPass,
			.framebuffer = m_imageFramebuffers[m_renderContext.targetSurface->currentTargetIndex()],
			.renderArea = { .extent = { .width = m_renderContext.targetSurface->properties().width,
										.height = m_renderContext.targetSurface->properties().height } },
			.clearValueCount = m_backgroundClearColor != Vector4(0.0f),
			.pClearValues = &clearValue
		};
		vkCmdBeginRenderPass(targetCommandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkClearAttachment clearAttachment = { .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
											  .clearValue = { .depthStencil = { .depth = 1.0f } } };
		VkClearRect clearRect = { .rect = beginInfo.renderArea, .baseArrayLayer = 0, .layerCount = 1 };
		vkCmdClearAttachments(targetCommandBuffer, 1, &clearAttachment, 1, &clearRect);

		for (auto& registry : m_shapeRegistries) {
			registry.second->renderShapes(targetCommandBuffer, nodeContext.frameIndex, m_uiPassSignature);
		}

		vkCmdEndRenderPass(targetCommandBuffer);
	}

	void UIRendererNode::handleWindowResize(graphics::FramegraphContext* context, uint32_t width, uint32_t height) {
		context->recreateImageResource(
			m_depthBufferHandle, { .imageType = VK_IMAGE_TYPE_2D,
								   .format = VK_FORMAT_D32_SFLOAT,
								   .extent = { .width = context->renderContext().targetSurface->properties().width,
											   .height = context->renderContext().targetSurface->properties().height,
											   .depth = 1 },
								   .mipLevels = 1,
								   .arrayLayers = 1,
								   .samples = VK_SAMPLE_COUNT_1_BIT,
								   .tiling = VK_IMAGE_TILING_OPTIMAL });

		for (auto& framebuffer : m_imageFramebuffers) {
			vkDestroyFramebuffer(m_renderContext.deviceContext->device(), framebuffer, nullptr);
		}
		m_imageFramebuffers.clear();

		m_imageFramebuffers.reserve(m_renderContext.targetSurface->currentImageCount());

		uint32_t imageIndex = 0;
		for (uint32_t i = 0; i < m_renderContext.targetSurface->currentImageCount(); ++i) {
			VkFramebuffer framebuffer;
			VkImageView attachmentViews[2] = { m_renderContext.targetSurface->targetView(i,
																						 m_attachmentResourceViewInfo),
											   context->imageView(this, m_depthBufferHandle, 0) };
			VkFramebufferCreateInfo uiFramebufferCreateInfo = { .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
																.renderPass = m_uiRenderPass,
																.attachmentCount = 2,
																.pAttachments = attachmentViews,
																.width =
																	m_renderContext.targetSurface->properties().width,
																.height =
																	m_renderContext.targetSurface->properties().height,
																.layers = 1 };
			vkCreateFramebuffer(m_renderContext.deviceContext->device(), &uiFramebufferCreateInfo, nullptr,
								&framebuffer);
			if constexpr (vanadiumGPUDebug) {
				setObjectName(m_renderContext.deviceContext->device(), VK_OBJECT_TYPE_FRAMEBUFFER, framebuffer,
							  "UI rendering framebuffer for image " + std::to_string(imageIndex));
				++imageIndex;
			}
			m_imageFramebuffers.push_back(framebuffer);
		}
	}

	void UIRendererNode::removeShape(Shape* shape) { m_shapeRegistries[shape->typenameHash()]->removeShape(shape); }

	void UIRendererNode::destroy(graphics::FramegraphContext* context) {
		for (auto& registryPair : m_shapeRegistries) {
			registryPair.second->destroy(m_uiPassSignature);
			delete registryPair.second;
		}
		for (auto& framebuffer : m_imageFramebuffers) {
			vkDestroyFramebuffer(m_renderContext.deviceContext->device(), framebuffer, nullptr);
		}
		vkDestroyRenderPass(m_renderContext.deviceContext->device(), m_uiRenderPass, nullptr);
	}
} // namespace vanadium::ui
