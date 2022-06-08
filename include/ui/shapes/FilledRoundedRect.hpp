
#include <graphics/RenderContext.hpp>
#include <ui/ShapeRegistry.hpp>
#include <ui/SimpleShapeDataManager.hpp>
#include <vector>

namespace vanadium::ui {
	class UISubsystem;
}

namespace vanadium::ui::shapes {

	class FilledRoundedRectShape;

	class FilledRoundedRectShapeRegistry : public ShapeRegistry {
	  public:
		FilledRoundedRectShapeRegistry(UISubsystem*, const graphics::RenderContext& context, VkRenderPass uiRenderPass,
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
			// From 0 (no rounded edge, entire edge is straight) to 1 (entire edge is rounded, no straight segment)
			float edgeSize;
		};
		struct PushConstantData {
			Vector2 targetDimensions;
			uint32_t instanceOffset;
		};

		uint32_t m_rectPipelineID;
		SimpleShapeDataManager<ShapeData> m_dataManager;

		graphics::RenderContext m_context;
		std::vector<FilledRoundedRectShape*> m_shapes;
	};

	class FilledRoundedRectShape : public Shape {
	  public:
		using ShapeRegistry = FilledRoundedRectShapeRegistry;

		FilledRoundedRectShape(Vector2 pos, uint32_t layerIndex, Vector2 size, float rotation, float edgeSize,
							   Vector4 color)
			: Shape("Filled Rounded Rect", layerIndex, pos, rotation), m_edgeSize(edgeSize), m_size(size),
			  m_color(color) {}

		const Vector2& size() const { return m_size; }
		const Vector4& color() const { return m_color; }
		float edgeSize() const { return m_edgeSize; }
		void setSize(const Vector2& size);
		void setColor(const Vector4& color);
		void setEdgeSize(float edgeSize);

	  private:
		float m_edgeSize;
		Vector2 m_size;
		Vector4 m_color;
	};

} // namespace vanadium::ui::shapes
