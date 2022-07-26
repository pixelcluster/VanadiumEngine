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
#include <ui/shapes/FilledRect.hpp>
#include <volk.h>
#include <ui/UISubsystem.hpp>

namespace vanadium::ui::shapes {

	FilledRectShapeRegistry::FilledRectShapeRegistry(UISubsystem* subsystem, const graphics::RenderContext& context,
													 VkRenderPass uiRenderPass,
													 const graphics::RenderPassSignature& uiRenderPassSignature)
		: m_rectPipelineID(context.pipelineLibrary->findGraphicsPipeline("UI Filled Rect")),
		  m_dataManager(context, m_rectPipelineID), m_subsystem(subsystem) {
		m_context = context;
		context.pipelineLibrary->createForPass(uiRenderPassSignature, uiRenderPass, { m_rectPipelineID });
	}

	void FilledRectShapeRegistry::addShape(Shape* shape) {
		FilledRectShape* rectShape = reinterpret_cast<FilledRectShape*>(shape);
		m_shapes.push_back(rectShape);
		m_dataManager.addShapeData(m_context, shape->layerIndex(),
								   { .position = rectShape->position(),
									 .size = rectShape->size(),
									 .color = rectShape->color(),
									 .cosSinRotation = { cosf(rectShape->rotation()), sinf(rectShape->rotation()) } });
		m_maxLayer = std::max(m_maxLayer, shape->layerIndex());
	}

	void FilledRectShapeRegistry::removeShape(Shape* shape) {
		auto iterator = std::find(m_shapes.begin(), m_shapes.end(), reinterpret_cast<FilledRectShape*>(shape));
		if (iterator != m_shapes.end()) {
			m_shapes.erase(iterator);
			m_dataManager.eraseShapeData(iterator - m_shapes.begin());
		}
	}

	void FilledRectShapeRegistry::prepareFrame(uint32_t frameIndex) {
		size_t shapeIndex = 0;
		bool anyShapeDirty = false;
		m_maxLayer = 0;
		for (auto& shape : m_shapes) {
			if (shape->dirtyFlag()) {
				m_dataManager.updateShapeData(
					shapeIndex, shape->layerIndex(),
					{ .position = shape->position(),
					  .size = shape->size(),
					  .color = shape->color(),
					  .cosSinRotation = { cosf(shape->rotation()), sinf(shape->rotation()) } });
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

	void FilledRectShapeRegistry::renderShapes(VkCommandBuffer commandBuffer, uint32_t frameIndex, uint32_t layerIndex,
											   const graphics::RenderPassSignature& uiRenderPassSignature) {
		auto layer = m_dataManager.layer(layerIndex);
		if (layer.elementCount == 0)
			return;

		auto scissorRect = m_subsystem->layerScissor(layerIndex);

		if (scissorRect.extent.width == 0 && scissorRect.extent.height == 0) {
			scissorRect.extent = { m_context.targetSurface->properties().width,
										 m_context.targetSurface->properties().height };
			scissorRect.offset = {};
		}

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
		vkCmdSetScissor(commandBuffer, 0, 1, &scissorRect);

		PushConstantData constantData = { .targetDimensions = Vector2(m_context.targetSurface->properties().width,
																	  m_context.targetSurface->properties().height),
										  .instanceOffset = layer.offset };
		VkShaderStageFlags stageFlags =
			m_context.pipelineLibrary->graphicsPipelinePushConstantRanges(m_rectPipelineID)[0].stageFlags;
		vkCmdPushConstants(commandBuffer, m_context.pipelineLibrary->graphicsPipelineLayout(m_rectPipelineID),
						   stageFlags, 0, sizeof(PushConstantData), &constantData);

		vkCmdDraw(commandBuffer, static_cast<uint32_t>(6U * layer.elementCount), 1, 0, 0);
	}

	void FilledRectShapeRegistry::destroy(const graphics::RenderPassSignature&) {
		for (auto& shape : m_shapes) {
			delete shape;
		}
		m_dataManager.destroy(m_context);
	}

	void FilledRectShape::setSize(const Vector2& size) {
		m_size = size;
		m_dirtyFlag = true;
	}

	void FilledRectShape::setColor(const Vector4& color) {
		m_color = color;
		m_dirtyFlag = true;
	}

} // namespace vanadium::ui::shapes
