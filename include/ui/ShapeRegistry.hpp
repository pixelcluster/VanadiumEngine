/* VanadiumEngine, a Vulkan rendering toolkit
 * Copyright (C) 2022 Friedrich Vock
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
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
