#pragma once

#include <modules/gpu/framegraph/VFramegraphNode.hpp>

class HelloTriangleNode : public VFramegraphNode {
  public:
	void setupResources(VFramegraphContext* context) override;

	void recordCommands(VFramegraphContext* context, VkCommandBuffer targetCommandBuffer,
						const VFramegraphNodeContext& nodeContext) override;

	void handleWindowResize(VFramegraphContext* context, uint32_t width, uint32_t height) override;

  private:
	VkPipeline m_graphicsPipeline;
	VkPipelineLayout m_pipelineLayout;
	VkRenderPass m_renderPass;

	uint32_t m_width, m_height;
	std::vector<VkFramebuffer> m_framebuffers;
};