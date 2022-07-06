#pragma once

#include <algorithm>
#include <graphics/RenderContext.hpp>
#include <ui/ShapeRegistry.hpp>
#include <util/Vector.hpp>
#include <volk.h>

namespace vanadium::ui {

	// Manages and organizes data for simple shapes (that don't need advanced batching or the like).
	// Also provides automatic ordering so that all data for a specific layer is in one place. This allows for shapes to
	// be drawn with one call per layer.
	//
	// Shape data is identified by the index of its shape in the registry's shape array.
	// For this, it is assumed that new shapes will always be appended at the end and never move.
	template <typename T> class SimpleShapeDataManager {
	  public:
		SimpleShapeDataManager(const graphics::RenderContext& context, uint32_t pipelineID);

		void addShapeData(const graphics::RenderContext& context, uint32_t layer, T&& t);
		void updateShapeData(size_t index, uint32_t layer, T&& t);

		// Rebuilds the CPU-side data buffer, but doesn't upload anything. Call only when data changed.
		void rebuildDataBuffer(size_t frameIndex);
		// Uploads the current CPU-side data buffer to the current GPU side data buffer if needed, does nothing
		// otherwise.
		void uploadDataBuffer(const graphics::RenderContext& context, size_t frameIndex);

		const VkDescriptorSet& frameDescriptorSet(size_t frameIndex) const { return m_shapeDataSets[frameIndex]; }

		void eraseShapeData(size_t index);

		void destroy(const graphics::RenderContext& context);

		RenderedLayer layer(uint32_t layerIndex);

	  private:
		void allocateBuffer(const graphics::RenderContext& context);
		constexpr static size_t m_initialShapeDataCapacity = 50;

		struct ShapeMetadata {
			uint32_t layerIndex;
			T data;
		};

		size_t m_descriptorSetRevisionCount[graphics::frameInFlightCount];
		size_t m_bufferRevisionCount;

		size_t m_maxShapeDataCapacity;
		graphics::GPUTransferHandle m_shapeDataTransfer;
		SimpleVector<ShapeMetadata> m_shapeData;

		// Cache for the most recent result of rebuilding the data buffer, since there is one GPU buffer for each frame
		// in flight and all of them need to be updated
		SimpleVector<T> m_shapeDataBuffer;
		SimpleVector<RenderedLayer> m_renderedLayers;

		uint32_t m_lastDataUpdateFrameIndex;

		// The frame index of the last data update is set in rebuildDataBuffer, uploadDataBuffer would have no way of
		// distinguishing whether the data was updated in the current frame or if the frame index wrapped around (which
		// would mean all buffers are sufficiently updated). If this variable is true, the data was updated in the
		// current frame.
		bool m_dataUpdatedThisFrame = false;

		VkDescriptorSet m_shapeDataSets[graphics::frameInFlightCount];
		SimpleVector<graphics::DescriptorSetAllocation> m_shapeDataSetAllocations;
		graphics::DescriptorSetAllocationInfo m_setAllocationInfo;
	};

	template <typename T>
	SimpleShapeDataManager<T>::SimpleShapeDataManager(const graphics::RenderContext& context, uint32_t pipelineID) {
		m_setAllocationInfo = { .typeInfos = { { .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .count = 1 } },
								.layout = context.pipelineLibrary->graphicsPipelineSet(pipelineID, 0).layout };
		m_shapeDataSetAllocations = context.descriptorSetAllocator->allocateDescriptorSets(
			SimpleVector<graphics::DescriptorSetAllocationInfo>(graphics::frameInFlightCount, m_setAllocationInfo));
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
		m_shapeData.push_back({ .layerIndex = layer, .data = std::forward<T>(t) });

		if (m_shapeData.size() > m_maxShapeDataCapacity) {
			m_maxShapeDataCapacity *= 1.61;
			m_maxShapeDataCapacity = std::max(m_shapeData.size(), m_maxShapeDataCapacity);
			context.transferManager->destroyTransfer(m_shapeDataTransfer);
			allocateBuffer(context);
		}
	}

	template <typename T> void SimpleShapeDataManager<T>::updateShapeData(size_t index, uint32_t layer, T&& t) {
		m_shapeData[index] = { .layerIndex = layer, .data = std::forward<T>(t) };
	}

	template <typename T> void SimpleShapeDataManager<T>::rebuildDataBuffer(size_t frameIndex) {
		// In order to be referencible by the shape index, the shape data cannot be sorted by layer index.
		// Here, we sort the shape data by its layer indices and then copy their data entries into the array to be
		// sent to the GPU.

		SimpleVector<ShapeMetadata> sortedShapeData = m_shapeData;
		std::sort(sortedShapeData.begin(), sortedShapeData.end(),
				  [](const auto& one, const auto& other) { return one.layerIndex < other.layerIndex; });

		// reserve maximum shape data capacity so the transfer won't read out-of-bounds
		m_shapeDataBuffer.clear();
		m_shapeDataBuffer.reserve(m_maxShapeDataCapacity);

		for (auto& data : sortedShapeData)
			m_shapeDataBuffer.push_back(data.data);

		// Regenerate list of renderable layers
		auto dataComparator = [](const auto& one, const auto& other) { return one.layerIndex < other; };

		m_renderedLayers.clear();

		uint32_t layerIndex = 0;

		auto offsetIterator =
			std::lower_bound(sortedShapeData.begin(), sortedShapeData.end(), layerIndex, dataComparator);
		auto nextOffsetIterator =
			std::lower_bound(sortedShapeData.begin(), sortedShapeData.end(), layerIndex + 1, dataComparator);
		while (offsetIterator != sortedShapeData.end()) {
			m_renderedLayers.push_back({ .offset = static_cast<uint32_t>(offsetIterator - sortedShapeData.begin()),
										 .elementCount = static_cast<uint32_t>(nextOffsetIterator - offsetIterator) });

			++layerIndex;
			offsetIterator =
				std::lower_bound(sortedShapeData.begin(), sortedShapeData.end(), layerIndex, dataComparator);
			nextOffsetIterator =
				std::lower_bound(sortedShapeData.begin(), sortedShapeData.end(), layerIndex + 1, dataComparator);
		}

		m_lastDataUpdateFrameIndex = frameIndex;
		m_dataUpdatedThisFrame = true;
	}

	template <typename T>
	void SimpleShapeDataManager<T>::uploadDataBuffer(const graphics::RenderContext& context, size_t frameIndex) {
		if (m_lastDataUpdateFrameIndex == ~0U)
			return;

		context.transferManager->updateTransferData(m_shapeDataTransfer, frameIndex, m_shapeDataBuffer.data());

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

		if (m_lastDataUpdateFrameIndex == frameIndex && !m_dataUpdatedThisFrame)
			m_lastDataUpdateFrameIndex = ~0U;

		m_dataUpdatedThisFrame = false;
	}

	template <typename T> void SimpleShapeDataManager<T>::eraseShapeData(size_t index) {
		m_shapeData.erase(m_shapeData.begin() + index);
	}

	template <typename T> void SimpleShapeDataManager<T>::destroy(const graphics::RenderContext& context) {
		for (auto& allocation : m_shapeDataSetAllocations) {
			context.descriptorSetAllocator->freeDescriptorSet(allocation, m_setAllocationInfo);
		}
	}

	template <typename T> RenderedLayer SimpleShapeDataManager<T>::layer(uint32_t layerIndex) {
		if (layerIndex < m_renderedLayers.size()) {
			return m_renderedLayers[layerIndex];
		} else {
			return RenderedLayer{};
		}
	}
} // namespace vanadium::ui
