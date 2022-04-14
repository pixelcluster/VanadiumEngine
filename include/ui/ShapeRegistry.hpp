#pragma once

#include <ui/Shape.hpp>

namespace vanadium::ui {
	class ShapeRegistry {
	  public:
		virtual void addShape(Shape* shape, uint32_t childDepth) = 0;
		virtual void removeShape(Shape* shape) = 0;
		virtual void prepareFrame(uint32_t frameIndex) = 0;
		virtual void renderShapes(VkCommandBuffer commandBuffers, uint32_t frameIndex, uint32_t childDepth, const graphics::RenderPassSignature& uiRenderPassSignature) = 0;
		virtual void destroy(const graphics::RenderPassSignature& uiRenderPassSignature) = 0;
		uint32_t maxChildDepth() const { return m_maxChildDepth; }

	  protected:
		uint32_t m_maxChildDepth = 0;
	};
} // namespace vanadium::ui
