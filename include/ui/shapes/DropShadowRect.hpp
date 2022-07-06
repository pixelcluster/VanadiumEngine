#pragma once

#include <graphics/RenderContext.hpp>
#include <ui/ShapeRegistry.hpp>
#include <ui/SimpleShapeDataManager.hpp>
#include <util/Vector.hpp>

namespace vanadium::ui {
	class UISubsystem;
}

namespace vanadium::ui::shapes {
	class DropShadowRectShape;

	class DropShadowRectShapeRegistry : public ShapeRegistry {
	  public:
		DropShadowRectShapeRegistry(UISubsystem* subsystem, const graphics::RenderContext& context, VkRenderPass uiRenderPass,
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
			// The coordinate space is:
			//  0 - - - - -x- - - - - 1
			//  -----------------------  0
			//  |.....................|  |
			//  |........shape........|  y
			//  |.....................|  |
			//  -----------------------  1
			Vector2 dropShadowPosition;
			float cosSinRotation[2];
			float maxOpacity;
			float _pad;
		};
		struct PushConstantData {
			Vector2 targetDimensions;
			uint32_t instanceOffset;
		};

		uint32_t m_rectPipelineID;
		SimpleShapeDataManager<ShapeData> m_dataManager;

		graphics::RenderContext m_context;
		SimpleVector<DropShadowRectShape*> m_shapes;
		UISubsystem* m_subsystem;
	};

	class DropShadowRectShape : public Shape {
	  public:
		using ShapeRegistry = DropShadowRectShapeRegistry;

		DropShadowRectShape(Vector2 pos, uint32_t layerIndex, Vector2 size, float rotation,
							const Vector2& shadowPeakPos, float maxOpacity)
			: Shape("Drop Shadow Rect", layerIndex, pos, rotation), m_size(size), m_shadowPeakPos(shadowPeakPos), m_maxOpacity(maxOpacity) {}
		const Vector2& size() const { return m_size; }
		const Vector2& shadowPeakPos() const { return m_shadowPeakPos; }
		float maxOpacity() const { return m_maxOpacity; }

		void setSize(const Vector2& size);
		void setShadowPeakPos(const Vector2& shadowPeakPos);
		void setMaxOpacity(float maxOpacity);

	  private:
		Vector2 m_size;
		Vector2 m_shadowPeakPos;
		float m_maxOpacity;
	};
} // namespace vanadium::ui::shapes
