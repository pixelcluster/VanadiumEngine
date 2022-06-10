#include <ui/shapes/DropShadowRect.hpp>
#include <volk.h>

namespace vanadium::ui::shapes {

	DropShadowRectShapeRegistry::DropShadowRectShapeRegistry(UISubsystem*, const graphics::RenderContext& context,
															 VkRenderPass uiRenderPass,
															 const graphics::RenderPassSignature& uiRenderPassSignature)
		: m_rectPipelineID(context.pipelineLibrary->findGraphicsPipeline("UI Drop Shadow Rect")),
		  m_dataManager(context, m_rectPipelineID) {
		m_context = context;
		context.pipelineLibrary->createForPass(uiRenderPassSignature, uiRenderPass, { m_rectPipelineID });
	}

	void DropShadowRectShapeRegistry::addShape(Shape* shape) {
		DropShadowRectShape* rectShape = reinterpret_cast<DropShadowRectShape*>(shape);
		m_shapes.push_back(rectShape);
		m_dataManager.addShapeData(m_context, shape->layerIndex(),
								   { .position = rectShape->position(),
									 .size = rectShape->size(),
									 .dropShadowPosition = rectShape->shadowPeakPos(),
									 .cosSinRotation = { cosf(rectShape->rotation()), sinf(rectShape->rotation()) },
									 .maxOpacity = rectShape->maxOpacity() });
		m_maxLayer = std::max(m_maxLayer, shape->layerIndex());
	}

	void DropShadowRectShapeRegistry::removeShape(Shape* shape) {
		auto iterator = std::find(m_shapes.begin(), m_shapes.end(), reinterpret_cast<DropShadowRectShape*>(shape));
		if (iterator != m_shapes.end()) {
			m_shapes.erase(iterator);
			m_dataManager.eraseShapeData(iterator - m_shapes.begin());
		}
	}

	void DropShadowRectShapeRegistry::prepareFrame(uint32_t frameIndex) {
		size_t shapeIndex = 0;
		bool anyShapeDirty = false;
		m_maxLayer = 0;
		for (auto& shape : m_shapes) {
			if (shape->dirtyFlag()) {
				m_dataManager.updateShapeData(shapeIndex, shape->layerIndex(),
											  { .position = shape->position(),
												.size = shape->size(),
												.dropShadowPosition = shape->shadowPeakPos(),
												.cosSinRotation = { cosf(shape->rotation()), sinf(shape->rotation()) },
												.maxOpacity = shape->maxOpacity() });
				shape->clearDirtyFlag();
				anyShapeDirty = true;
			}
			m_maxLayer = std::max(m_maxLayer, shape->layerIndex());
			++shapeIndex;
		}

		if (anyShapeDirty)
			m_dataManager.rebuildDataBuffer(frameIndex);
		m_dataManager.uploadDataBuffer(m_context, frameIndex);
	}

	void DropShadowRectShapeRegistry::renderShapes(VkCommandBuffer commandBuffer, uint32_t frameIndex,
												   uint32_t layerIndex,
												   const graphics::RenderPassSignature& uiRenderPassSignature) {
		auto layer = m_dataManager.layer(layerIndex);
		if (layer.elementCount == 0)
			return;
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

		PushConstantData constantData = { .targetDimensions = Vector2(m_context.targetSurface->properties().width,
																	  m_context.targetSurface->properties().height),
										  .instanceOffset = layer.offset };
		VkShaderStageFlags stageFlags =
			m_context.pipelineLibrary->graphicsPipelinePushConstantRanges(m_rectPipelineID)[0].stageFlags;
		vkCmdPushConstants(commandBuffer, m_context.pipelineLibrary->graphicsPipelineLayout(m_rectPipelineID),
						   stageFlags, 0, sizeof(PushConstantData), &constantData);

		vkCmdDraw(commandBuffer, static_cast<uint32_t>(6U * layer.elementCount), 1, 0, 0);
	}

	void DropShadowRectShapeRegistry::destroy(const graphics::RenderPassSignature&) {
		for (auto& shape : m_shapes) {
			delete shape;
		}
		m_dataManager.destroy(m_context);
	}

	void DropShadowRectShape::setSize(const Vector2& size) {
		m_size = size;
		m_dirtyFlag = true;
	}

	void DropShadowRectShape::setShadowPeakPos(const Vector2& shadowPeakPos) {
		m_shadowPeakPos = shadowPeakPos;
		m_dirtyFlag = true;
	}

	void DropShadowRectShape::setMaxOpacity(float maxOpacity) {
		m_maxOpacity = maxOpacity;
		m_dirtyFlag = true;
	}

} // namespace vanadium::ui::shapes
