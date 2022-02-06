#pragma once

#include <modules/gpu/framegraph/VFramegraphContext.hpp>

class VFramegraphNode {
  public:
	virtual void setupResources(VFramegraphContext* context) = 0;

	virtual void recordCommands(VFramegraphContext* context, VkCommandBuffer targetCommandBuffer, const VFramegraphNodeContext& nodeContext) = 0;

	virtual void handleWindowResize(VFramegraphContext* context, uint32_t width, uint32_t height) {}

	virtual void destroyResources(VFramegraphContext* context) = 0;

	const std::string& name() const { return m_name; }

  protected:
	std::string m_name;
};