#pragma once

#include <modules/gpu/framegraph/VFramegraphContext.hpp>

class VFramegraphNode {
  public:
	virtual void setupResources(VFramegraphContext* context) = 0;

	virtual void recordCommands(VFramegraphContext* context, VkCommandBuffer targetCommandBuffer, const VFramegraphFrameInfo& frameInfo,
								const std::unordered_map<std::string, VkImageView> imageViewHandles) = 0;

	virtual void handleWindowResize(VFramegraphContext* context, uint32_t width, uint32_t height) {}
};