#pragma once

#include <ui/Shape.hpp>

namespace vanadium::ui {

	class ShapeRegistry {
	  public:
		virtual ~ShapeRegistry() {}
		virtual void addShape(Shape* shape) = 0;
		virtual void removeShape(Shape* shape) = 0;
		virtual void renderShapes(VkCommandBuffer commandBuffers, uint32_t frameIndex,
								  const graphics::RenderPassSignature& uiRenderPassSignature) = 0;
		virtual void destroy(const graphics::RenderPassSignature& uiRenderPassSignature) = 0;

		virtual void handleWindowResize(uint32_t width, uint32_t height) {}
	};
} // namespace vanadium::ui
