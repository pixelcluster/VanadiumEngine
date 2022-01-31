#pragma once

#include <modules/gpu/framegraph/VFramegraphNode.hpp>

class VSwapchainClearNode : public VFramegraphNode {
  public:
	VSwapchainClearNode();

	void setupResources(VFramegraphContext* context) override;

	void recordCommands(VFramegraphContext* context, VkCommandBuffer targetCommandBuffer) override;
  private:
}