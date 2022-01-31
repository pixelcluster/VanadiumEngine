#pragma once

#include <modules/gpu/framegraph/VFramegraphNode.hpp>

class SwapchainClearNode : public VFramegraphNode {
  public:
	SwapchainClearNode();

	void setupResources(VFramegraphContext* context) override;

	void recordCommands(VFramegraphContext* context, VkCommandBuffer targetCommandBuffer) override;

  private:
};