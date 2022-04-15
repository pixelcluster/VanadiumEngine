#pragma once

#include <graphics/framegraph/FramegraphNode.hpp>
#include <graphics/util/GPUTransferManager.hpp>
#include <VertexBufferUpdater.hpp>

class HelloTriangleNode : public vanadium::graphics::FramegraphNode {
  public:
	HelloTriangleNode(VertexBufferUpdater* bufferUpdater);

	void create(vanadium::graphics::FramegraphContext* context) override {}

	void setupResources(vanadium::graphics::FramegraphContext* context) override;

	void initResources(vanadium::graphics::FramegraphContext* context) override {}

	void recordCommands(vanadium::graphics::FramegraphContext* context, VkCommandBuffer targetCommandBuffer,
						const vanadium::graphics::FramegraphNodeContext& nodeContext) override;

	void handleWindowResize(vanadium::graphics::FramegraphContext* context, uint32_t width, uint32_t height) override;

	void destroy(vanadium::graphics::FramegraphContext* context) override;

  private:
	VertexBufferUpdater* m_bufferUpdater;
	vanadium::graphics::RenderPassSignature m_passSignature;
	VkRenderPass m_renderPass;
	uint32_t m_pipelineID;

	vanadium::graphics::ImageResourceViewInfo m_swapchainViewInfo;

	uint32_t m_width, m_height;
	std::vector<VkFramebuffer> m_framebuffers;
};