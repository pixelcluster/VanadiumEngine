#include <ui/shapes/FilledRect.hpp>
#include <volk.h>

namespace vanadium::ui::shapes {

	auto childDepthComparator = [](const auto& one, const auto& other) { return one.childDepth < other.childDepth; };

	FilledRectShapeRegistry::FilledRectShapeRegistry(UISubsystem*, const graphics::RenderContext& context, VkRenderPass uiRenderPass,
													 const graphics::RenderPassSignature& uiRenderPassSignature)
		: m_rectPipelineID(context.pipelineLibrary->findGraphicsPipeline("UI Filled Rect")),
		  m_dataManager(context, m_rectPipelineID) {
		m_context = context;
		context.pipelineLibrary->createForPass(uiRenderPassSignature, uiRenderPass, { m_rectPipelineID });
	}

	void FilledRectShapeRegistry::addShape(Shape* shape, uint32_t childDepth) {
		FilledRectShape* rectShape = reinterpret_cast<FilledRectShape*>(shape);

		auto iterator =
			std::lower_bound(m_shapes.begin(), m_shapes.end(),
							 FilledRectShapeRegistry::RegistryEntry{ .childDepth = childDepth }, childDepthComparator);
		if (iterator == m_shapes.end()) {
			m_shapes.push_back({ .childDepth = childDepth, .shape = rectShape });
			m_dataManager.insertShapeData(
				m_context, m_shapes.size() - 1,
				{ .position = rectShape->position(), .size = rectShape->size(), .color = rectShape->color() });
		} else {
			size_t index = iterator - m_shapes.begin();
			m_shapes.insert(iterator, { .childDepth = childDepth, .shape = rectShape });
			m_dataManager.insertShapeData(
				m_context, index,
				{ .position = rectShape->position(), .size = rectShape->size(), .color = rectShape->color() });
		}

		m_maxChildDepth = std::max(m_maxChildDepth, childDepth);
	}

	void FilledRectShapeRegistry::removeShape(Shape* shape) {
		auto iterator = std::find_if(m_shapes.begin(), m_shapes.end(),
									 [shape](const auto& element) { return element.shape == shape; });
		if (iterator != m_shapes.end()) {
			m_shapes.erase(iterator);
			m_dataManager.eraseShapeData(iterator - m_shapes.begin());
		}
	}

	void FilledRectShapeRegistry::prepareFrame(uint32_t frameIndex) {
		size_t shapeIndex = 0;
		for (auto& shapeData : m_shapes) {
			m_dataManager.updateShapeData(shapeIndex, { .position = shapeData.shape->position(),
														.size = shapeData.shape->size(),
														.color = shapeData.shape->color() });
			++shapeIndex;
		}
		m_dataManager.prepareFrame(m_context, frameIndex);
	}

	void FilledRectShapeRegistry::renderShapes(VkCommandBuffer commandBuffer, uint32_t frameIndex, uint32_t childDepth,
											   const graphics::RenderPassSignature& uiRenderPassSignature) {
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

		auto rangeIterator =
			std::lower_bound(m_shapes.begin(), m_shapes.end(),
							 FilledRectShapeRegistry::RegistryEntry{ .childDepth = childDepth }, childDepthComparator);
		auto rangeEndIterator = std::lower_bound(m_shapes.begin(), m_shapes.end(),
												 FilledRectShapeRegistry::RegistryEntry{ .childDepth = childDepth + 1 },
												 childDepthComparator);
		uint32_t elementCount = rangeEndIterator - rangeIterator;
		uint32_t elementOffset = rangeIterator - m_shapes.begin();
		vkCmdDraw(commandBuffer, 6, elementCount, 0, elementOffset);
	}

	void FilledRectShapeRegistry::destroy(const graphics::RenderPassSignature&) {
		for (auto& shape : m_shapes) {
			delete shape.shape;
		}
		m_dataManager.destroy(m_context);
	}

} // namespace vanadium::ui::shapes
