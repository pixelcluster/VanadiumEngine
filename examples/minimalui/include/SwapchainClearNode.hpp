#pragma once

#include <graphics/framegraph/FramegraphNode.hpp>

class SwapchainClearNode : public vanadium::graphics::FramegraphNode {
  public:
	SwapchainClearNode() { m_name = "Swapchain clearing"; }

	void create(vanadium::graphics::FramegraphContext* context) override {}

	void setupResources(vanadium::graphics::FramegraphContext* context) override;

	void initResources(vanadium::graphics::FramegraphContext* context) override {}

	void recordCommands(vanadium::graphics::FramegraphContext* context, VkCommandBuffer targetCommandBuffer,
						const vanadium::graphics::FramegraphNodeContext& nodeContext) override;

	void destroy(vanadium::graphics::FramegraphContext* context) override {}

  private:
};