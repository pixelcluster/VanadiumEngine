#pragma once

#include <modules/gpu/framegraph/VFramegraphNode.hpp>
#include <modules/gpu/transfer/VGPUTransferManager.hpp>

class HelloTriangleNode : public VFramegraphNode {
  public:
	HelloTriangleNode(VBufferResourceHandle vertexDataHandle);

	void create(VFramegraphContext* context) override {}

	void setupResources(VFramegraphContext* context) override;

	void initResources(VFramegraphContext* context) override {}

	void recordCommands(VFramegraphContext* context, VkCommandBuffer targetCommandBuffer,
						const VFramegraphNodeContext& nodeContext) override;

	void handleWindowResize(VFramegraphContext* context, uint32_t width, uint32_t height) override;

	void destroy(VFramegraphContext* context) override;

  private:
	VBufferResourceHandle m_vertexData;
	VkPipeline m_graphicsPipeline;
	VkPipelineLayout m_pipelineLayout;
	VkRenderPass m_renderPass;

	uint32_t m_width, m_height;
	std::vector<VkFramebuffer> m_framebuffers;
};