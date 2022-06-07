#pragma once

#include <algorithm>
#include <graphics/RenderContext.hpp>
#include <vector>
#include <volk.h>
#include <ui/ShapeRegistry.hpp>

namespace vanadium::ui {

	template <typename T> class SimpleShapeDataManager {
	  public:
		SimpleShapeDataManager(const graphics::RenderContext& context, uint32_t pipelineID);

		void addShapeData(const graphics::RenderContext& context, uint32_t layer, T&& t);
		void updateShapeData(size_t index, T&& t);

		void prepareFrame(const graphics::RenderContext& context, size_t frameIndex);
		const VkDescriptorSet& frameDescriptorSet(size_t frameIndex) const { return m_shapeDataSets[frameIndex]; }

		void eraseShapeData(size_t index);

		void destroy(const graphics::RenderContext& context);

		RenderedLayer layer(uint32_t layerIndex);

	  private:
		void allocateBuffer(const graphics::RenderContext& context);
		constexpr static size_t m_initialShapeDataCapacity = 50;

		struct ShapeMetadata {
			uint32_t layerIndex;
			uint32_t dataArrayIndex;
		};

		size_t m_descriptorSetRevisionCount[graphics::frameInFlightCount];
		size_t m_bufferRevisionCount;

		size_t m_maxShapeDataCapacity;
		graphics::GPUTransferHandle m_shapeDataTransfer;
		std::vector<T> m_shapeData;
		std::vector<ShapeMetadata> m_shapeMetadata;

		VkDescriptorSet m_shapeDataSets[graphics::frameInFlightCount];
		std::vector<graphics::DescriptorSetAllocation> m_shapeDataSetAllocations;
		graphics::DescriptorSetAllocationInfo m_setAllocationInfo;
	};

	template <typename T>
	SimpleShapeDataManager<T>::SimpleShapeDataManager(const graphics::RenderContext& context, uint32_t pipelineID) {
		m_setAllocationInfo = { .typeInfos = { { .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .count = 1 } },
								.layout = context.pipelineLibrary->graphicsPipelineSet(pipelineID, 0).layout };
		m_shapeDataSetAllocations = context.descriptorSetAllocator->allocateDescriptorSets(
			std::vector<graphics::DescriptorSetAllocationInfo>(graphics::frameInFlightCount, m_setAllocationInfo));
		for (uint32_t i = 0; i < graphics::frameInFlightCount; ++i) {
			m_shapeDataSets[i] = m_shapeDataSetAllocations[i].set;
			m_descriptorSetRevisionCount[i] = 0;
		}

		m_maxShapeDataCapacity = m_initialShapeDataCapacity;

		allocateBuffer(context);
	}

	template <typename T> void SimpleShapeDataManager<T>::allocateBuffer(const graphics::RenderContext& context) {
		++m_bufferRevisionCount;
		m_shapeData.reserve(m_maxShapeDataCapacity);
		m_shapeDataTransfer = context.transferManager->createTransfer(
			m_maxShapeDataCapacity * sizeof(T), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
			VK_ACCESS_SHADER_READ_BIT);
	}

	template <typename T>
	void SimpleShapeDataManager<T>::addShapeData(const graphics::RenderContext& context, uint32_t layer, T&& t) {
		auto shapeIterator =
			std::lower_bound(m_shapeMetadata.begin(), m_shapeMetadata.end(), layer + 1,
							 [](const auto& one, const auto& other) { return one.layerIndex < other; });
		auto shapeIndex = shapeIterator - m_shapeMetadata.begin();
		m_shapeMetadata.push_back({ .layerIndex = layer, .dataArrayIndex = static_cast<uint32_t>(shapeIndex) });
		if (shapeIndex == m_shapeData.size()) {
			m_shapeData.push_back(t);
		} else {
			m_shapeData.insert(m_shapeData.begin() + shapeIndex, t);
		}

		if (m_shapeData.size() > m_maxShapeDataCapacity) {
			m_maxShapeDataCapacity *= 1.61;
			m_maxShapeDataCapacity = std::max(m_shapeData.size(), m_maxShapeDataCapacity);
			context.transferManager->destroyTransfer(m_shapeDataTransfer);
			allocateBuffer(context);
		}
	}

	template <typename T> void SimpleShapeDataManager<T>::updateShapeData(size_t index, T&& t) {
		m_shapeData[m_shapeMetadata[index].dataArrayIndex] = std::forward<T>(t);
	}

	template <typename T>
	void SimpleShapeDataManager<T>::prepareFrame(const graphics::RenderContext& context, size_t frameIndex) {
		context.transferManager->updateTransferData(m_shapeDataTransfer, frameIndex, m_shapeData.data());

		if (m_bufferRevisionCount > m_descriptorSetRevisionCount[frameIndex]) {
			VkDescriptorBufferInfo bufferInfo = { .buffer = context.resourceAllocator->nativeBufferHandle(
													  context.transferManager->dstBufferHandle(m_shapeDataTransfer)),
												  .offset = 0,
												  .range = VK_WHOLE_SIZE };
			VkWriteDescriptorSet writeDescriptorSet = { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
														.dstSet = m_shapeDataSets[frameIndex],
														.dstBinding = 0,
														.dstArrayElement = 0,
														.descriptorCount = 1,
														.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
														.pBufferInfo = &bufferInfo };
			vkUpdateDescriptorSets(context.deviceContext->device(), 1, &writeDescriptorSet, 0, nullptr);
			m_descriptorSetRevisionCount[frameIndex] = m_bufferRevisionCount;
		}
	}

	template <typename T> void SimpleShapeDataManager<T>::eraseShapeData(size_t index) {
		m_shapeData.erase(m_shapeData.begin() + m_shapeMetadata[index].dataArrayIndex);
		m_shapeMetadata.erase(m_shapeMetadata.begin() + index);
	}

	template <typename T> void SimpleShapeDataManager<T>::destroy(const graphics::RenderContext& context) {
		for (auto& allocation : m_shapeDataSetAllocations) {
			context.descriptorSetAllocator->freeDescriptorSet(allocation, m_setAllocationInfo);
		}
	}

	template <typename T> RenderedLayer SimpleShapeDataManager<T>::layer(uint32_t layerIndex) {
		auto offsetIterator =
			std::lower_bound(m_shapeMetadata.begin(), m_shapeMetadata.end(), layerIndex,
							 [](const auto& one, const auto& other) { return one.layerIndex < other; });
		auto nextOffsetIterator =
			std::lower_bound(m_shapeMetadata.begin(), m_shapeMetadata.end(), layerIndex + 1,
							 [](const auto& one, const auto& other) { return one.layerIndex < other; });
		return { .offset = static_cast<uint32_t>(offsetIterator - m_shapeMetadata.begin()),
				 .elementCount = static_cast<uint32_t>(nextOffsetIterator - offsetIterator) };
	}
} // namespace vanadium::ui
