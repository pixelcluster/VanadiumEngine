#pragma once

#include <graphics/framegraph/FramegraphNode.hpp>

class SwapchainClearNode : public FramegraphNode {
  public:
	SwapchainClearNode() { m_name = "Swapchain clearing"; }

	void create(FramegraphContext* context) override {}

	void setupResources(FramegraphContext* context) override;

	void initResources(FramegraphContext* context) override {}

	void recordCommands(FramegraphContext* context, VkCommandBuffer targetCommandBuffer,
						const FramegraphNodeContext& nodeContext) override;

	void destroy(FramegraphContext* context) override {}

  private:
};