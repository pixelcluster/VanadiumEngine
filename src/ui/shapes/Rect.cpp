#include <ui/shapes/Rect.hpp>
#include <volk.h>

namespace vanadium::ui::shapes {

	auto childDepthComparator = [](const auto& one, const auto& other) { return one.childDepth < other.childDepth; };

	RectShapeRegistry::RectShapeRegistry(const graphics::RenderContext& context, VkRenderPass uiRenderPass,
										 const graphics::RenderPassSignature& uiRenderPassSignature) {
		m_context = context;
		m_rectPipelineID = context.pipelineLibrary->findGraphicsPipeline("UI Rect");
		context.pipelineLibrary->createForPass(uiRenderPassSignature, uiRenderPass, { m_rectPipelineID });
		m_setAllocationInfo = { .typeInfos = { { .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .count = 1 } },
								.layout = context.pipelineLibrary->graphicsPipelineSet(m_rectPipelineID, 0).layout };
		m_shapeDataSetAllocations = context.descriptorSetAllocator->allocateDescriptorSets(
			std::vector<graphics::DescriptorSetAllocationInfo>(graphics::frameInFlightCount, m_setAllocationInfo));
		for (uint32_t i = 0; i < graphics::frameInFlightCount; ++i) {
			m_shapeDataSets[i] = m_shapeDataSetAllocations[i].set;
			m_descriptorSetRevisionCount[i] = 0;
		}

		m_maxShapeDataCapacity = m_initialShapeDataCapacity;

		allocateBuffer();
	}

	void RectShapeRegistry::allocateBuffer() {
		++m_bufferRevisionCount;
		m_shapeDataTransfer = m_context.transferManager->createTransfer(
			m_maxShapeDataCapacity * sizeof(ShapeData), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT);
	}

	void RectShapeRegistry::addShape(Shape* shape, uint32_t childDepth) {
		RectShape* rectShape = reinterpret_cast<RectShape*>(shape);

		auto iterator =
			std::lower_bound(m_shapes.begin(), m_shapes.end(),
							 RectShapeRegistry::RegistryEntry{ .childDepth = childDepth }, childDepthComparator);
		if (iterator == m_shapes.end()) {
			m_shapes.push_back({ .childDepth = childDepth, .shape = rectShape });
		} else {
			m_shapes.insert(iterator, { .childDepth = childDepth, .shape = rectShape });
		}
		m_shapeData.push_back(
			{ .position = rectShape->position(), .size = rectShape->size(), .color = rectShape->color() });

		if (m_shapes.size() > m_maxShapeDataCapacity) {
			m_maxShapeDataCapacity *= 2;
			m_context.transferManager->destroyTransfer(m_shapeDataTransfer);
			allocateBuffer();
		}

		m_maxChildDepth = std::max(m_maxChildDepth, childDepth);
	}

	void RectShapeRegistry::removeShape(Shape* shape) {
		auto iterator = std::find_if(m_shapes.begin(), m_shapes.end(),
									 [shape](const auto& element) { return element.shape == shape; });
		if (iterator != m_shapes.end()) {
			m_shapes.erase(iterator);
			m_shapeData.erase(m_shapeData.begin() + (iterator - m_shapes.begin()));
		}
	}

	void RectShapeRegistry::prepareFrame(uint32_t frameIndex) {
		size_t shapeIndex = 0;
		for (auto& shapeData : m_shapes) {
			m_shapeData[shapeIndex] = { .position = shapeData.shape->position(),
										.size = shapeData.shape->size(),
										.color = shapeData.shape->color() };
			++shapeIndex;
		}
		m_context.transferManager->updateTransferData(m_shapeDataTransfer, frameIndex, m_shapeData.data());

		if (m_bufferRevisionCount > m_descriptorSetRevisionCount[frameIndex]) {
			VkDescriptorBufferInfo bufferInfo = { .buffer = m_context.resourceAllocator->nativeBufferHandle(
													  m_context.transferManager->dstBufferHandle(m_shapeDataTransfer,
																								 frameIndex)),
												  .offset = 0,
												  .range = VK_WHOLE_SIZE };
			VkWriteDescriptorSet writeDescriptorSet = { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
														.dstSet = m_shapeDataSets[frameIndex],
														.dstBinding = 0,
														.dstArrayElement = 0,
														.descriptorCount = 1,
														.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
														.pBufferInfo = &bufferInfo };
			vkUpdateDescriptorSets(m_context.deviceContext->device(), 1, &writeDescriptorSet, 0, nullptr);
			m_descriptorSetRevisionCount[frameIndex] = m_bufferRevisionCount;
		}
	}

	void RectShapeRegistry::renderShapes(VkCommandBuffer commandBuffer, uint32_t frameIndex, uint32_t childDepth,
										 const graphics::RenderPassSignature& uiRenderPassSignature) {
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
						  m_context.pipelineLibrary->graphicsPipeline(m_rectPipelineID, uiRenderPassSignature));
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
								m_context.pipelineLibrary->graphicsPipelineLayout(m_rectPipelineID), 0, 1,
								&m_shapeDataSets[frameIndex], 0, nullptr);
		VkViewport viewport = { .width = static_cast<float>(m_context.targetSurface->properties().width),
								.height = static_cast<float>(m_context.targetSurface->properties().height),
								.minDepth = 0.0f,
								.maxDepth = 1.0f };
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		auto rangeIterator =
			std::lower_bound(m_shapes.begin(), m_shapes.end(),
							 RectShapeRegistry::RegistryEntry{ .childDepth = childDepth }, childDepthComparator);
		auto rangeEndIterator =
			std::lower_bound(m_shapes.begin(), m_shapes.end(),
							 RectShapeRegistry::RegistryEntry{ .childDepth = childDepth + 1 }, childDepthComparator);
		uint32_t elementCount = rangeEndIterator - rangeIterator;
		uint32_t elementOffset = rangeIterator - m_shapes.begin();
		vkCmdDraw(commandBuffer, 6, elementCount, 0, elementOffset);
	}

	void RectShapeRegistry::destroy(const graphics::RenderPassSignature&) {
		for (auto& allocation : m_shapeDataSetAllocations) {
			m_context.descriptorSetAllocator->freeDescriptorSet(allocation, m_setAllocationInfo);
		}
		for (auto& shape : m_shapes) {
			delete shape.shape;
		}
	}

} // namespace vanadium::ui::shapes
