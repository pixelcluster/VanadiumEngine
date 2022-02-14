#pragma once

#include <modules/gpu/framegraph/VFramegraphNode.hpp>

class SwapchainClearNode : public VFramegraphNode {
  public:
	SwapchainClearNode() { m_name = "Swapchain clearing"; }

	void create(VFramegraphContext* context) override {}

	void setupResources(VFramegraphContext* context) override;

	void initResources(VFramegraphContext* context) override {}

	void recordCommands(VFramegraphContext* context, VkCommandBuffer targetCommandBuffer,
						const VFramegraphNodeContext& nodeContext) override;

	void destroy(VFramegraphContext* context) override {}

  private:
};