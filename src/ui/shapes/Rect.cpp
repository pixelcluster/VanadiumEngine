#include <ui/shapes/Rect.hpp>
#include <volk.h>

namespace vanadium::ui::shapes {

	RectShapeRegistry::RectShapeRegistry(UISubsystem*, const graphics::RenderContext& context,
										 VkRenderPass uiRenderPass,
										 const graphics::RenderPassSignature& uiRenderPassSignature)
		: m_rectPipelineID(context.pipelineLibrary->findGraphicsPipeline("UI Rect")),
		  m_dataManager(context, m_rectPipelineID) {
		m_context = context;
		context.pipelineLibrary->createForPass(uiRenderPassSignature, uiRenderPass, { m_rectPipelineID });
	}

	void RectShapeRegistry::addShape(Shape* shape) {
		RectShape* rectShape = reinterpret_cast<RectShape*>(shape);
		m_shapes.push_back(rectShape);
		m_dataManager.addShapeData(m_context,
								   { .position = rectShape->position(),
									 .color = rectShape->color(),
									 .size = rectShape->size(),
									 .cosSinRotation = { cosf(rectShape->rotation()), sinf(rectShape->rotation()) } });
	}

	void RectShapeRegistry::removeShape(Shape* shape) {
		auto iterator = std::find(m_shapes.begin(), m_shapes.end(), reinterpret_cast<RectShape*>(shape));
		if (iterator != m_shapes.end()) {
			m_shapes.erase(iterator);
			m_dataManager.eraseShapeData(iterator - m_shapes.begin());
		}
	}

	void RectShapeRegistry::renderShapes(VkCommandBuffer commandBuffer, uint32_t frameIndex,
										 const graphics::RenderPassSignature& uiRenderPassSignature) {
		size_t shapeIndex = 0;
		for (auto& shape : m_shapes) {
			m_dataManager.updateShapeData(shapeIndex,
										  { .position = shape->position(),
											.color = shape->color(),
											.size = shape->size(),
											.cosSinRotation = { cosf(shape->rotation()), sinf(shape->rotation()) } });
			++shapeIndex;
		}
		m_dataManager.prepareFrame(m_context, frameIndex);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
						  m_context.pipelineLibrary->graphicsPipeline(m_rectPipelineID, uiRenderPassSignature));
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
								m_context.pipelineLibrary->graphicsPipelineLayout(m_rectPipelineID), 0, 1,
								&m_dataManager.frameDescriptorSet(frameIndex), 0, nullptr);
		VkViewport viewport = { .width = static_cast<float>(m_context.targetSurface->properties().width),
								.height = static_cast<float>(m_context.targetSurface->properties().height),
								.minDepth = 0.0f,
								.maxDepth = 1.0f };
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		Vector2 windowSize =
			Vector2(m_context.targetSurface->properties().width, m_context.targetSurface->properties().height);
		VkShaderStageFlags stageFlags =
			m_context.pipelineLibrary->graphicsPipelinePushConstantRanges(m_rectPipelineID)[0].stageFlags;
		vkCmdPushConstants(commandBuffer, m_context.pipelineLibrary->graphicsPipelineLayout(m_rectPipelineID),
						   stageFlags, 0, sizeof(Vector2), &windowSize);
		vkCmdDraw(commandBuffer, static_cast<uint32_t>(8U * m_shapes.size()), 1, 0, 0);
	}

	void RectShapeRegistry::destroy(const graphics::RenderPassSignature&) {
		for (auto& shape : m_shapes) {
			delete shape;
		}
		m_dataManager.destroy(m_context);
	}

} // namespace vanadium::ui::shapes
