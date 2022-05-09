#pragma once

#include <ui/Shape.hpp>

namespace vanadium::ui {

	struct RenderedLayer {
		uint32_t offset;
		uint32_t elementCount;
	};

	class ShapeRegistry {
	  public:
		virtual ~ShapeRegistry() {}
		virtual void addShape(Shape* shape) = 0;
		virtual void removeShape(Shape* shape) = 0;
		virtual void prepareFrame(uint32_t frameIndex) = 0;
		virtual void renderShapes(VkCommandBuffer commandBuffers, uint32_t frameIndex, uint32_t layerIndex,
								  const graphics::RenderPassSignature& uiRenderPassSignature) = 0;
		virtual void destroy(const graphics::RenderPassSignature& uiRenderPassSignature) = 0;

		virtual void handleWindowResize(uint32_t width, uint32_t height) {}

		uint32_t maxLayer() const { return m_maxLayer; }

	  protected:
		uint32_t m_maxLayer = 0;
	};
} // namespace vanadium::ui
