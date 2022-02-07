#pragma once

#include <modules/gpu/framegraph/VFramegraphNode.hpp>

class SwapchainClearNode : public VFramegraphNode {
  public:
	SwapchainClearNode() { m_name = "Swapchain clearing"; }

	void setupResources(VFramegraphContext* context) override;

	void initResources(VFramegraphContext* context) override {}

	void recordCommands(VFramegraphContext* context, VkCommandBuffer targetCommandBuffer,
						const VFramegraphNodeContext& nodeContext) override;

	void destroyResources(VFramegraphContext* context) override {}

  private:
};