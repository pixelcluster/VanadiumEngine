#pragma once

#include <graphics/framegraph/FramegraphNode.hpp>
#include <graphics/util/GPUTransferManager.hpp>

class HelloTriangleNode : public FramegraphNode {
  public:
	HelloTriangleNode(BufferResourceHandle vertexDataHandle);

	void create(FramegraphContext* context) override {}

	void setupResources(FramegraphContext* context) override;

	void initResources(FramegraphContext* context) override {}

	void recordCommands(FramegraphContext* context, VkCommandBuffer targetCommandBuffer,
						const FramegraphNodeContext& nodeContext) override;

	void handleWindowResize(FramegraphContext* context, uint32_t width, uint32_t height) override;

	void destroy(FramegraphContext* context) override;

  private:
	BufferResourceHandle m_vertexData;
	VkPipeline m_graphicsPipeline;
	VkPipelineLayout m_pipelineLayout;
	VkRenderPass m_renderPass;

	uint32_t m_width, m_height;
	std::vector<VkFramebuffer> m_framebuffers;
};