#include <graphics/helper/DebugHelper.hpp>
#include <graphics/helper/ErrorHelper.hpp>
#include <ui/UIRendererNode.hpp>
#include <volk.h>

namespace vanadium::ui {
	void UIRendererNode::create(graphics::FramegraphContext* context) {
		m_framegraphContext = context;
		m_swapchainImageResourceViewInfo = {
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
			.viewInfos = { m_swapchainImageResourceViewInfo }
		});
	}

	void UIRendererNode::initResources(graphics::FramegraphContext* context) {
		VkAttachmentDescription targetAttachmentDescription = {
			.format = m_renderContext.targetSurface->properties().format,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = m_backgroundClearColor != Vector4(0.f) ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE,
			.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
		};
		VkAttachmentReference targetAttachmentReference = { .attachment = 0,
															.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
		VkSubpassDescription subpassDescription = { .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
													.colorAttachmentCount = 1,
													.pColorAttachments = &targetAttachmentReference };
		VkRenderPassCreateInfo createInfo = { .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
											  .attachmentCount = 1,
											  .pAttachments = &targetAttachmentDescription,
											  .subpassCount = 1,
											  .pSubpasses = &subpassDescription };
		verifyResult(
			vkCreateRenderPass(m_renderContext.deviceContext->device(), &createInfo, nullptr, &m_uiRenderPass));
		m_uiPassSignature = graphics::RenderPassSignature{
			.subpassSignatures = { { .outputAttachments = { { .isUsed = true,
															  .format =
																  m_renderContext.targetSurface->properties().format,
															  .sampleCount = VK_SAMPLE_COUNT_1_BIT } } } },
			.attachmentDescriptionSignatures = { { .isUsed = true,
												   .format = m_renderContext.targetSurface->properties().format,
												   .sampleCount = VK_SAMPLE_COUNT_1_BIT } }
		};
	}

	void UIRendererNode::recordCommands(graphics::FramegraphContext* context, VkCommandBuffer targetCommandBuffer,
										const graphics::FramegraphNodeContext& nodeContext) {
		if (nodeContext.frameIndex == m_framebufferDeletionFrameIndex) {
			for (auto& framebuffer : m_oldImageFramebuffers) {
				vkDestroyFramebuffer(m_renderContext.deviceContext->device(), framebuffer, nullptr);
			}
			m_oldImageFramebuffers.clear();
			m_framebufferDeletionFrameIndex = ~0U;
		}
		for (auto& registry : m_shapeRegistries) {
			registry.second->prepareFrame(nodeContext.frameIndex);
		}

		uint32_t maxChildLevel = 0;
		for (auto& registry : m_shapeRegistries) {
			maxChildLevel = std::max(maxChildLevel, registry.second->maxChildDepth());
		}

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

		for (uint32_t i = 0; i <= maxChildLevel; ++i) {
			for (auto& registry : m_shapeRegistries) {
				registry.second->renderShapes(targetCommandBuffer, nodeContext.frameIndex, i, m_uiPassSignature);
			}
		}

		vkCmdEndRenderPass(targetCommandBuffer);
		m_lastRenderedFrameIndex = nodeContext.frameIndex;
	}

	void UIRendererNode::handleWindowResize(graphics::FramegraphContext* context, uint32_t width, uint32_t height) {
		if (!m_imageFramebuffers.empty()) {
			m_oldImageFramebuffers.reserve(m_oldImageFramebuffers.size() + m_imageFramebuffers.size());

			for(auto& framebuffer : m_imageFramebuffers) {
				m_oldImageFramebuffers.push_back(framebuffer);
			}

			m_framebufferDeletionFrameIndex = m_lastRenderedFrameIndex;
		}
		m_imageFramebuffers.clear();

		m_imageFramebuffers.reserve(m_renderContext.targetSurface->currentImageCount());

		uint32_t imageIndex = 0;
		for (uint32_t i = 0; i < m_renderContext.targetSurface->currentImageCount(); ++i) {
			VkFramebuffer framebuffer;
			VkImageView swapchainImageView =
				m_renderContext.targetSurface->targetView(i, m_swapchainImageResourceViewInfo);
			VkFramebufferCreateInfo uiFramebufferCreateInfo = { .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
																.renderPass = m_uiRenderPass,
																.attachmentCount = 1,
																.pAttachments = &swapchainImageView,
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
		}
		for (auto& framebuffer : m_imageFramebuffers) {
			vkDestroyFramebuffer(m_renderContext.deviceContext->device(), framebuffer, nullptr);
		}
		vkDestroyRenderPass(m_renderContext.deviceContext->device(), m_uiRenderPass, nullptr);
	}
} // namespace vanadium::ui
