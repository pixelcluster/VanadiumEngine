#pragma once

#include <graphics/framegraph/FramegraphContext.hpp>

namespace vanadium::graphics {

	class FramegraphNode {
	  public:
		virtual void create(FramegraphContext* context) = 0;

		virtual void setupResources(FramegraphContext* context) = 0;

		virtual void initResources(FramegraphContext* context) = 0;

		virtual void recordCommands(FramegraphContext* context, VkCommandBuffer targetCommandBuffer,
									const FramegraphNodeContext& nodeContext) = 0;

		virtual void handleWindowResize(FramegraphContext* context, uint32_t width, uint32_t height) {}

		virtual void destroy(FramegraphContext* context) = 0;

		const std::string& name() const { return m_name; }

	  protected:
		std::string m_name;
	};

} // namespace vanadium::graphics