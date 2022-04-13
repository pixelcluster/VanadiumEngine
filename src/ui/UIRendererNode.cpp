#include <graphics/helper/DebugHelper.hpp>
#include <graphics/helper/ErrorHelper.hpp>
#include <ui/UIRendererNode.hpp>
#include <volk.h>

namespace vanadium::ui {
	void UIRendererNode::create(graphics::FramegraphContext* context) {
		m_framegraphContext = context;
		VkAttachmentDescription targetAttachmentDescription = {
			.format = context->renderContext().targetSurface->properties().format,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
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
		verifyResult(vkCreateRenderPass(context->renderContext().deviceContext->device(), &createInfo, nullptr,
										&m_uiRenderPass));
		m_uiPassSignature = graphics::RenderPassSignature{
			.subpassSignatures = { { .outputAttachments = { { .isUsed = true,
															  .format = context->renderContext()
																			.targetSurface->properties()
																			.format,
															  .sampleCount = VK_SAMPLE_COUNT_1_BIT } } } },
			.attachmentDescriptionSignatures = { { .isUsed = true,
												   .format =
													   context->renderContext().targetSurface->properties().format,
												   .sampleCount = VK_SAMPLE_COUNT_1_BIT } }
		};
	}

	void UIRendererNode::setupResources(graphics::FramegraphContext* context) {
		context->declareReferencedSwapchainImage(this, {
			.subresourceAccesses = {
				graphics::NodeImageSubresourceAccess {
					.accessingPipelineStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
					.access = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
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
			.usageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			.writes = true,
			.viewInfos = { m_swapchainImageResourceViewInfo }
		});
	}

	void UIRendererNode::initResources(graphics::FramegraphContext* context) {}

	void UIRendererNode::recordCommands(graphics::FramegraphContext* context, VkCommandBuffer targetCommandBuffer,
										const graphics::FramegraphNodeContext& nodeContext) {

		for (auto& registry : m_shapeRegistries) {
			registry.second->prepareFrame(nodeContext.frameIndex);
		}

		uint32_t maxChildLevel = 0;
		for (auto& registry : m_shapeRegistries) {
			maxChildLevel = std::max(maxChildLevel, registry.second->maxChildDepth());
		}

		VkRenderPassBeginInfo beginInfo = {
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.renderPass = m_uiRenderPass,
			.framebuffer = m_imageFramebuffers[context->renderContext().targetSurface->currentTargetIndex()],
			.renderArea = { .extent = { .width = context->renderContext().targetSurface->properties().width,
										.height = context->renderContext().targetSurface->properties().height } }
		};
		vkCmdBeginRenderPass(targetCommandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

		for (uint32_t i = 0; i <= maxChildLevel; ++i) {
			for (auto& registry : m_shapeRegistries) {
				registry.second->renderShapes(targetCommandBuffer, nodeContext.frameIndex, i, m_uiPassSignature);
			}
		}
	}

	void UIRendererNode::handleWindowResize(graphics::FramegraphContext* context, uint32_t width, uint32_t height) {
		for (auto& framebuffer : m_imageFramebuffers) {
			vkDestroyFramebuffer(context->renderContext().deviceContext->device(), framebuffer, nullptr);
		}

		m_imageFramebuffers.reserve(context->renderContext().targetSurface->currentImageCount());

		uint32_t imageIndex = 0;
		for (uint32_t i = 0; i < context->renderContext().targetSurface->currentImageCount(); ++i) {
			VkFramebuffer framebuffer;
			VkImageView swapchainImageView =
				context->renderContext().targetSurface->targetView(i, m_swapchainImageResourceViewInfo);
			VkFramebufferCreateInfo uiFramebufferCreateInfo = {
				.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
				.renderPass = m_uiRenderPass,
				.attachmentCount = 1,
				.pAttachments = &swapchainImageView,
				.width = context->renderContext().targetSurface->properties().width,
				.height = context->renderContext().targetSurface->properties().height,
				.layers = 1
			};
			vkCreateFramebuffer(context->renderContext().deviceContext->device(), &uiFramebufferCreateInfo, nullptr,
								&framebuffer);
			if constexpr (vanadiumGPUDebug) {
				setObjectName(context->renderContext().deviceContext->device(), VK_OBJECT_TYPE_FRAMEBUFFER, framebuffer,
							  "UI rendering framebuffer for image " + std::to_string(imageIndex));
				++imageIndex;
			}
		}
	}

	void UIRendererNode::removeShape(Shape* shape) { m_shapeRegistries[shape->typenameHash()]->removeShape(shape); }
} // namespace vanadium::ui
