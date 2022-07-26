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

#include <graphics/RenderContext.hpp>
#include <ui/ShapeRegistry.hpp>
#include <ui/SimpleShapeDataManager.hpp>
#include <vector>

namespace vanadium::ui {
	class UISubsystem;
}

namespace vanadium::ui::shapes {

	class FilledRectShape;

	class FilledRectShapeRegistry : public ShapeRegistry {
	  public:
		FilledRectShapeRegistry(UISubsystem* subsystem, const graphics::RenderContext& context, VkRenderPass uiRenderPass,
								const graphics::RenderPassSignature& uiRenderPassSignature);

		void addShape(Shape* shape) override;
		void removeShape(Shape* shape) override;
		void prepareFrame(uint32_t frameIndex) override;
		void renderShapes(VkCommandBuffer commandBuffers, uint32_t frameIndex, uint32_t layerIndex,
						  const graphics::RenderPassSignature& uiRenderPassSignature) override;
		void destroy(const graphics::RenderPassSignature& uiRenderPassSignature) override;

	  private:
		struct ShapeData {
			Vector2 position;
			Vector2 size;
			Vector4 color;
			float cosSinRotation[2];
			float _pad[2];
		};
		struct PushConstantData {
			Vector2 targetDimensions;
			uint32_t instanceOffset;
		};

		uint32_t m_rectPipelineID;
		SimpleShapeDataManager<ShapeData> m_dataManager;

		graphics::RenderContext m_context;
		std::vector<FilledRectShape*> m_shapes;
		UISubsystem* m_subsystem;
	};

	class FilledRectShape : public Shape {
	  public:
		using ShapeRegistry = FilledRectShapeRegistry;

		FilledRectShape(Vector2 pos, uint32_t layerIndex, Vector2 size, float rotation, Vector4 color)
			: Shape("Filled Rect", layerIndex, pos, rotation), m_size(size), m_color(color) {}

		const Vector2& size() const { return m_size; }
		const Vector4& color() const { return m_color; }
		void setSize(const Vector2& size);
		void setColor(const Vector4& color);

	  private:
		Vector2 m_size;
		Vector4 m_color;
	};

} // namespace vanadium::ui::shapes
