#pragma once

#include <modules/gpu/framegraph/VFramegraphNode.hpp>

class HelloTriangleNode : public VFramegraphNode {
  public:
	void setupResources(VFramegraphContext* context) override;

	void recordCommands(VFramegraphContext* context, VkCommandBuffer targetCommandBuffer) override;

	void handleWindowResize(VFramegraphContext* context) override;

  private:
	VkPipeline m_graphicsPipeline;
	VkPipelineLayout m_pipelineLayout;
	VkRenderPass m_renderPass;

	std::vector<VkFramebuffer> m_framebuffers;
};