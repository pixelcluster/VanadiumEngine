#include <graphics/framegraph/QueueBarrierGenerator.hpp>

// true if [offset1; offset1 + size1] overlaps with [offset2; offset2 + size2]
template <typename T, T fullRangeValue> bool overlaps(T offset1, T size1, T offset2, T size2) {
	if (size1 == fullRangeValue) {
		if (size2 == fullRangeValue) {
			return true;
		} else {
			return offset1 < offset2 + size2;
		}
	} else if (size2 == fullRangeValue) {
		return offset1 + size1 > offset2;
	} else {
		return offset1 >= offset2 && offset1 < offset2 + size2;
	}
}

// true if [offset2; offset2 + size2] is completely contained within [offset1; offset1 + size1]
template <typename T, T fullRangeValue> bool isCompletelyContained(T offset1, T size1, T offset2, T size2) {
	if (size1 == fullRangeValue) {
		if (size2 == fullRangeValue) {
			return true;
		} else {
			return offset1 <= offset2;
		}
	} else if (size2 == fullRangeValue) {
		return false;
	} else {
		return offset1 <= offset2 && offset1 + size1 >= offset2 + size2;
	}
}

namespace vanadium::graphics {
	void QueueBarrierGenerator::create(size_t nodeCount) { m_nodeBarrierInfos.resize(nodeCount); }

	void QueueBarrierGenerator::addNodeBufferAccess(size_t nodeIndex, const NodeBufferAccess& bufferAccess) {
		for (auto& subresourceAccess : bufferAccess.subresourceAccesses) {
			if (subresourceAccess.writes) {
				m_bufferAccessInfos[bufferAccess.buffer].modifications.push_back(
					{ .accessingPipelineStages = subresourceAccess.accessingPipelineStages,
					  .access = subresourceAccess.access,
					  .offset = subresourceAccess.offset,
					  .size = subresourceAccess.size,
					  .nodeIndex = nodeIndex });
			} else {
				m_bufferAccessInfos[bufferAccess.buffer].reads.push_back(
					{ .accessingPipelineStages = subresourceAccess.accessingPipelineStages,
					  .access = subresourceAccess.access,
					  .offset = subresourceAccess.offset,
					  .size = subresourceAccess.size,
					  .nodeIndex = nodeIndex });
			}
		}
	}

	void QueueBarrierGenerator::addNodeImageAccess(size_t nodeIndex, const NodeImageAccess& imageAccess) {
		for (auto& subresourceAccess : imageAccess.subresourceAccesses) {
			if (subresourceAccess.writes) {
				m_imageAccessInfos[imageAccess.image].modifications.push_back(
					{ .accessingPipelineStages = subresourceAccess.accessingPipelineStages,
					  .access = subresourceAccess.access,
					  .subresourceRange = subresourceAccess.subresourceRange,
					  .startLayout = subresourceAccess.startLayout,
					  .finishLayout = subresourceAccess.finishLayout,
					  .nodeIndex = nodeIndex });
			} else {
				m_imageAccessInfos[imageAccess.image].reads.push_back(
					{ .accessingPipelineStages = subresourceAccess.accessingPipelineStages,
					  .access = subresourceAccess.access,
					  .subresourceRange = subresourceAccess.subresourceRange,
					  .startLayout = subresourceAccess.startLayout,
					  .finishLayout = subresourceAccess.finishLayout,
					  .nodeIndex = nodeIndex });
			}
		}
		m_imageAccessInfos[imageAccess.image].preserveAcrossFrames |= imageAccess.preserveAcrossFrames;
		m_imageAccessInfos[imageAccess.image].initialLayout = imageAccess.initialLayout;
	}

	void QueueBarrierGenerator::addNodeTargetImageAccess(size_t nodeIndex, const NodeImageAccess& imageAccess) {
		for (auto& subresourceAccess : imageAccess.subresourceAccesses) {
			if (subresourceAccess.writes) {
				m_targetAccessInfo.modifications.push_back(
					{ .accessingPipelineStages = subresourceAccess.accessingPipelineStages,
					  .access = subresourceAccess.access,
					  .subresourceRange = subresourceAccess.subresourceRange,
					  .startLayout = subresourceAccess.startLayout,
					  .finishLayout = subresourceAccess.finishLayout,
					  .nodeIndex = nodeIndex });
			} else {
				m_targetAccessInfo.reads.push_back(
					{ .accessingPipelineStages = subresourceAccess.accessingPipelineStages,
					  .access = subresourceAccess.access,
					  .subresourceRange = subresourceAccess.subresourceRange,
					  .startLayout = subresourceAccess.startLayout,
					  .finishLayout = subresourceAccess.finishLayout,
					  .nodeIndex = nodeIndex });
			}
		}
	}

	void QueueBarrierGenerator::generateDependencyInfo() {
		for (auto& nodeInfo : m_nodeBarrierInfos) {
			nodeInfo.bufferBarriers.clear();
			nodeInfo.imageBarriers.clear();
		}
		m_frameStartImageBarriers.clear();

		for (auto& buffer : m_bufferAccessInfos) {
			std::sort(buffer.second.reads.begin(), buffer.second.reads.end());
			std::sort(buffer.second.modifications.begin(), buffer.second.modifications.end());

			for (auto& read : buffer.second.reads) {
				emitBarriersForRead(read.nodeIndex, buffer.first, buffer.second, read);
			}
			for (auto& write : buffer.second.modifications) {
				emitBarriersForRead(write.nodeIndex, buffer.first, buffer.second, write);
			}
		}
		for (auto& image : m_imageAccessInfos) {
			std::sort(image.second.reads.begin(), image.second.reads.end());
			std::sort(image.second.modifications.begin(), image.second.modifications.end());

			for (auto& read : image.second.reads) {
				emitBarriersForRead(read.nodeIndex, image.first, image.second, read);
			}
			for (auto& write : image.second.modifications) {
				emitBarriersForRead(write.nodeIndex, image.first, image.second, write);
			}
		}

		std::sort(m_targetAccessInfo.reads.begin(), m_targetAccessInfo.reads.end());
		std::sort(m_targetAccessInfo.modifications.begin(), m_targetAccessInfo.modifications.end());

		for (auto& read : m_targetAccessInfo.reads) {
			emitBarriersForRead(read.nodeIndex, std::nullopt, m_targetAccessInfo, read);
		}
		for (auto& write : m_targetAccessInfo.modifications) {
			emitBarriersForRead(write.nodeIndex, std::nullopt, m_targetAccessInfo, write);
		}

		size_t nodeIndex = 0;
		for (auto& info : m_nodeBarrierInfos) {
			size_t firstDstNodeIndex = m_nodeBarrierInfos.size();
			for (auto& bufferBarrier : info.bufferBarriers) {
				firstDstNodeIndex = std::min(firstDstNodeIndex, bufferBarrier.dstNodeIndex);
			}
			for (auto& imageBarrier : info.imageBarriers) {
				firstDstNodeIndex = std::min(firstDstNodeIndex, imageBarrier.dstNodeIndex);
			}
			if (firstDstNodeIndex > nodeIndex + 1) {
				// move barriers into next node
				m_nodeBarrierInfos[nodeIndex + 1].bufferBarriers.insert(
					m_nodeBarrierInfos[nodeIndex + 1].bufferBarriers.end(),
					std::make_move_iterator(info.bufferBarriers.begin()),
					std::make_move_iterator(info.bufferBarriers.end()));
				info.bufferBarriers.clear();

				m_nodeBarrierInfos[nodeIndex + 1].imageBarriers.insert(
					m_nodeBarrierInfos[nodeIndex + 1].imageBarriers.end(),
					std::make_move_iterator(info.imageBarriers.begin()),
					std::make_move_iterator(info.imageBarriers.end()));
				info.imageBarriers.clear();
			}
			++nodeIndex;
		}
	}

	void QueueBarrierGenerator::generateBarrierInfo(bool initStage, BufferHandleRetriever bufferHandleRetriever,
													ImageHandleRetriever imageHandleRetriever,
													FramegraphContext* context, VkImage currentTargetImage) {
		m_vulkanFrameStartImageBarriers.reserve(m_frameStartImageBarriers.size());
		m_vulkanFrameStartImageBarriers.clear();

		for (auto& barrier : m_frameStartImageBarriers) {
			VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			if (barrier.image.has_value()) {
				initialLayout = m_imageAccessInfos[barrier.image.value()].initialLayout;
			}

			m_vulkanFrameStartImageBarriers.push_back(
				{ .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				  .srcAccessMask = barrier.srcAccessFlags,
				  .dstAccessMask = barrier.dstAccessFlags,
				  .oldLayout = initStage ? initialLayout : barrier.beforeLayout,
				  .newLayout = barrier.afterLayout,
				  .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				  .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				  .image = barrier.image.has_value() ? (context->*(imageHandleRetriever))(barrier.image.value())
													 : currentTargetImage,
				  .subresourceRange = barrier.subresourceRange });
		}

		for (auto& info : m_nodeBarrierInfos) {
			info.vulkanBufferBarriers.reserve(info.bufferBarriers.size());
			info.vulkanBufferBarriers.clear();
			info.vulkanImageBarriers.reserve(info.imageBarriers.size());
			info.vulkanImageBarriers.clear();

			for (auto& barrier : info.bufferBarriers) {
				info.vulkanBufferBarriers.push_back(
					VkBufferMemoryBarrier{ .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
										   .srcAccessMask = barrier.srcAccessFlags,
										   .dstAccessMask = barrier.dstAccessFlags,
										   .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
										   .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
										   .buffer = (context->*(bufferHandleRetriever))(barrier.buffer),
										   .offset = barrier.offset,
										   .size = barrier.size });
				info.srcStages |= barrier.srcPipelineStageFlags;
				info.dstStages |= barrier.dstPipelineStageFlags;
			}
			for (auto& barrier : info.imageBarriers) {
				info.vulkanImageBarriers.push_back(
					{ .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
					  .srcAccessMask = barrier.srcAccessFlags,
					  .dstAccessMask = barrier.dstAccessFlags,
					  .oldLayout = barrier.beforeLayout,
					  .newLayout = barrier.afterLayout,
					  .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					  .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					  .image = barrier.image.has_value() ? (context->*(imageHandleRetriever))(barrier.image.value())
														 : currentTargetImage,
					  .subresourceRange = barrier.subresourceRange });
				info.srcStages |= barrier.srcPipelineStageFlags;
				info.dstStages |= barrier.dstPipelineStageFlags;
			}
		}
	}

	size_t QueueBarrierGenerator::bufferBarrierCount(size_t nodeIndex) const {
		return m_nodeBarrierInfos[nodeIndex].vulkanBufferBarriers.size();
	}

	const std::vector<VkBufferMemoryBarrier>& QueueBarrierGenerator::bufferBarriers(size_t nodeIndex) const {
		return m_nodeBarrierInfos[nodeIndex].vulkanBufferBarriers;
	}

	size_t QueueBarrierGenerator::imageBarrierCount(size_t nodeIndex) const {
		return m_nodeBarrierInfos[nodeIndex].vulkanImageBarriers.size();
	}

	const std::vector<VkImageMemoryBarrier>& QueueBarrierGenerator::imageBarriers(size_t nodeIndex) const {
		return m_nodeBarrierInfos[nodeIndex].vulkanImageBarriers;
	}

	VkPipelineStageFlags QueueBarrierGenerator::srcStages(size_t nodeIndex) const {
		return m_nodeBarrierInfos[nodeIndex].srcStages;
	}

	VkPipelineStageFlags QueueBarrierGenerator::dstStages(size_t nodeIndex) const {
		return m_nodeBarrierInfos[nodeIndex].dstStages;
	}

	VkImageLayout QueueBarrierGenerator::lastTargetImageLayout() const {
		if (m_targetAccessInfo.reads.empty() && m_targetAccessInfo.modifications.empty())
			return VK_IMAGE_LAYOUT_UNDEFINED;
		bool isLastAccessRead;
		if (!m_targetAccessInfo.reads.empty() && !m_targetAccessInfo.modifications.empty()) {
			isLastAccessRead = m_targetAccessInfo.reads.back().nodeIndex > m_targetAccessInfo.reads.back().nodeIndex;
		} else {
			isLastAccessRead = m_targetAccessInfo.modifications.empty();
		}

		if (isLastAccessRead) {
			return m_targetAccessInfo.reads.back().finishLayout;
		} else if (!m_targetAccessInfo.modifications.empty()) {
			return m_targetAccessInfo.modifications.back().finishLayout;
		}
		return VK_IMAGE_LAYOUT_UNDEFINED;
	}

	VkImageSubresourceRange QueueBarrierGenerator::lastTargetAccessRange() const {
		if (m_targetAccessInfo.reads.empty() && m_targetAccessInfo.modifications.empty())
			return { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					 .baseMipLevel = 0,
					 .levelCount = 1,
					 .baseArrayLayer = 0,
					 .layerCount = 1 };
		bool isLastAccessRead;
		if (!m_targetAccessInfo.reads.empty() && !m_targetAccessInfo.modifications.empty()) {
			isLastAccessRead = m_targetAccessInfo.reads.back().nodeIndex > m_targetAccessInfo.reads.back().nodeIndex;
		} else {
			isLastAccessRead = m_targetAccessInfo.modifications.empty();
		}

		if (isLastAccessRead) {
			return m_targetAccessInfo.reads.back().subresourceRange;
		} else if (!m_targetAccessInfo.modifications.empty()) {
			return m_targetAccessInfo.modifications.back().subresourceRange;
		}
		return { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				 .baseMipLevel = 0,
				 .levelCount = 1,
				 .baseArrayLayer = 0,
				 .layerCount = 1 };
	}

	void QueueBarrierGenerator::emitBarriersForRead(size_t nodeIndex, SlotmapHandle buffer,
													const BufferAccessInfo& info, const BufferSubresourceAccess& read) {
		auto matchOptional = findLastModification(info.modifications, read);
		if (!matchOptional.has_value())
			return;
		auto& match = matchOptional.value();

		if (match.isPartial) {
			if (match.offset > read.offset) {
				BufferSubresourceAccess partialRead = read;
				partialRead.offset = read.offset;
				partialRead.size = match.offset - read.offset;
				emitBarriersForRead(nodeIndex, buffer, info, partialRead);
			}
			if (read.size == VK_WHOLE_SIZE && match.size != VK_WHOLE_SIZE) {
				BufferSubresourceAccess partialRead = read;
				partialRead.offset = match.offset + match.size;
				emitBarriersForRead(nodeIndex, buffer, info, partialRead);
			} else if (match.size != VK_WHOLE_SIZE && match.offset + match.size < read.offset + read.size) {
				BufferSubresourceAccess partialRead = read;
				partialRead.offset = match.offset + match.size;
				partialRead.size = (read.offset + read.size) - (partialRead.offset);
				emitBarriersForRead(nodeIndex, buffer, info, partialRead);
			}
		}

		emitBarrier(buffer, match, info.modifications[match.accessIndex], read);
	}
	void QueueBarrierGenerator::emitBarriersForRead(size_t nodeIndex, std::optional<SlotmapHandle> image,
													const ImageAccessInfo& info, const ImageSubresourceAccess& read) {
		auto matchOptional = findLastModification(info.modifications, read);
		if (!matchOptional.has_value()) {
			auto& lastModification = info.modifications.back();
			m_frameStartImageBarriers.push_back(
				{ .dstNodeIndex = nodeIndex,
				  .srcPipelineStageFlags = lastModification.accessingPipelineStages,
				  .dstPipelineStageFlags = read.accessingPipelineStages,
				  .srcAccessFlags = lastModification.access,
				  .dstAccessFlags = read.access,
				  .subresourceRange = read.subresourceRange,
				  .beforeLayout = info.preserveAcrossFrames ? lastModification.finishLayout : VK_IMAGE_LAYOUT_UNDEFINED,
				  .afterLayout = read.startLayout,
				  .image = image });
			return;
		}
		auto& match = matchOptional.value();

		if (match.isPartial) {
			if (match.range.aspectMask != read.subresourceRange.aspectMask) {
				ImageSubresourceAccess partialRead = read;
				partialRead.subresourceRange.aspectMask = read.subresourceRange.aspectMask & ~match.range.aspectMask;
				emitBarriersForRead(nodeIndex, image, info, partialRead);
			}

			if (match.range.baseArrayLayer > read.subresourceRange.baseArrayLayer) {
				ImageSubresourceAccess partialRead = read;
				partialRead.subresourceRange.baseArrayLayer = read.subresourceRange.baseArrayLayer;
				partialRead.subresourceRange.layerCount =
					match.range.baseArrayLayer - read.subresourceRange.baseArrayLayer;
				emitBarriersForRead(nodeIndex, image, info, partialRead);
			} else if (read.subresourceRange.layerCount == VK_REMAINING_ARRAY_LAYERS &&
					   match.range.layerCount != VK_REMAINING_ARRAY_LAYERS) {
				ImageSubresourceAccess partialRead = read;
				partialRead.subresourceRange.baseArrayLayer = match.range.baseArrayLayer + match.range.layerCount;
				emitBarriersForRead(nodeIndex, image, info, partialRead);
			} else if (match.range.layerCount != VK_REMAINING_ARRAY_LAYERS &&
					   match.range.baseArrayLayer + match.range.layerCount <
						   read.subresourceRange.baseArrayLayer + read.subresourceRange.layerCount) {
				ImageSubresourceAccess partialRead = read;
				partialRead.subresourceRange.baseArrayLayer = match.range.baseArrayLayer + match.range.layerCount;
				partialRead.subresourceRange.layerCount =
					(read.subresourceRange.baseArrayLayer + read.subresourceRange.layerCount) -
					(partialRead.subresourceRange.baseArrayLayer);
				emitBarriersForRead(nodeIndex, image, info, partialRead);
			}

			if (match.range.baseMipLevel > read.subresourceRange.baseMipLevel) {
				ImageSubresourceAccess partialRead = read;
				partialRead.subresourceRange.baseMipLevel = read.subresourceRange.baseMipLevel;
				partialRead.subresourceRange.levelCount = match.range.baseMipLevel - read.subresourceRange.baseMipLevel;
				emitBarriersForRead(nodeIndex, image, info, partialRead);
			} else if (read.subresourceRange.levelCount == VK_REMAINING_MIP_LEVELS &&
					   match.range.levelCount != VK_REMAINING_MIP_LEVELS) {
				ImageSubresourceAccess partialRead = read;
				partialRead.subresourceRange.baseMipLevel = match.range.baseMipLevel + match.range.levelCount;
				emitBarriersForRead(nodeIndex, image, info, partialRead);
			} else if (match.range.levelCount != VK_REMAINING_MIP_LEVELS &&
					   match.range.baseMipLevel + match.range.levelCount <
						   read.subresourceRange.baseMipLevel + read.subresourceRange.levelCount) {
				ImageSubresourceAccess partialRead = read;
				partialRead.subresourceRange.baseMipLevel = match.range.baseMipLevel + match.range.levelCount;
				partialRead.subresourceRange.levelCount =
					(read.subresourceRange.baseMipLevel + read.subresourceRange.levelCount) -
					(partialRead.subresourceRange.baseMipLevel);
				emitBarriersForRead(nodeIndex, image, info, partialRead);
			}
		}

		emitBarrier(image, match, info.modifications[match.accessIndex], read);
	}

	std::optional<BufferAccessMatch> QueueBarrierGenerator::findLastModification(
		const std::vector<BufferSubresourceAccess>& modifications, const BufferSubresourceAccess& read) {
		if (modifications.empty())
			return std::nullopt;
		auto nextModificationIterator = std::lower_bound(modifications.begin(), modifications.end(), read);

		if (nextModificationIterator == modifications.begin() || nextModificationIterator == modifications.end()) {
			return std::nullopt;
		}
		--nextModificationIterator;

		while (true) {
			if (overlaps<VkDeviceSize, VK_WHOLE_SIZE>(nextModificationIterator->offset, nextModificationIterator->size,
													  read.offset, read.size)) {
				// match was found
				BufferAccessMatch match = { .matchingNodeIndex = nextModificationIterator->nodeIndex,
											.accessIndex =
												static_cast<size_t>(nextModificationIterator - modifications.begin()) };
				if (isCompletelyContained<VkDeviceSize, VK_WHOLE_SIZE>(
						nextModificationIterator->offset, nextModificationIterator->size, read.offset, read.size)) {
					match.isPartial = false;
					match.offset = read.offset;
					match.size = read.size;
					return match;
				} else {
					match.isPartial = true;
					match.offset = std::max(read.offset, nextModificationIterator->offset);
					if (read.size == VK_WHOLE_SIZE) {
						if (nextModificationIterator->size == VK_WHOLE_SIZE) {
							match.size = VK_WHOLE_SIZE;
						} else {
							match.size =
								nextModificationIterator->offset + nextModificationIterator->size - match.offset;
						}
					} else if (nextModificationIterator->size == VK_WHOLE_SIZE) {
						match.size = read.offset + read.size - match.offset;
					} else {
						match.size = std::min(read.offset + read.size,
											  nextModificationIterator->offset + nextModificationIterator->size) -
									 match.offset;
					}
					return match;
				}
			}
			if (nextModificationIterator == modifications.begin()) {
				break;
			}
			--nextModificationIterator;
		}
		return std::nullopt;
	}
	std::optional<ImageAccessMatch> QueueBarrierGenerator::findLastModification(
		const std::vector<ImageSubresourceAccess>& modifications, const ImageSubresourceAccess& read) {
		if (modifications.empty())
			return std::nullopt;
		auto nextModificationIterator = std::lower_bound(modifications.begin(), modifications.end(), read);

		if (nextModificationIterator == modifications.begin() || nextModificationIterator == modifications.end()) {
			return std::nullopt;
		}
		--nextModificationIterator;
		while (true) {
			auto& srcRange = nextModificationIterator->subresourceRange;
			auto& dstRange = read.subresourceRange;
			bool resourceMatches = srcRange.aspectMask & dstRange.aspectMask;
			resourceMatches &= overlaps<uint32_t, VK_REMAINING_ARRAY_LAYERS>(
				srcRange.baseArrayLayer, srcRange.layerCount, dstRange.baseArrayLayer, dstRange.layerCount);
			resourceMatches &= overlaps<uint32_t, VK_REMAINING_MIP_LEVELS>(srcRange.baseMipLevel, srcRange.levelCount,
																		   dstRange.baseMipLevel, dstRange.levelCount);
			if (resourceMatches) {
				ImageAccessMatch match = { .matchingNodeIndex = nextModificationIterator->nodeIndex,
										   .accessIndex =
											   static_cast<size_t>(nextModificationIterator - modifications.begin()) };
				bool matchesFully = srcRange.aspectMask == dstRange.aspectMask;
				matchesFully &= isCompletelyContained<uint32_t, VK_REMAINING_ARRAY_LAYERS>(
					srcRange.baseArrayLayer, srcRange.layerCount, dstRange.baseArrayLayer, dstRange.layerCount);
				matchesFully &= isCompletelyContained<uint32_t, VK_REMAINING_MIP_LEVELS>(
					srcRange.baseMipLevel, srcRange.levelCount, dstRange.baseMipLevel, dstRange.levelCount);
				if (matchesFully) {
					match.isPartial = false;
					match.range = srcRange;
					return match;
				} else {
					match.isPartial = true;
					match.range.aspectMask = srcRange.aspectMask & dstRange.aspectMask;
					match.range.baseArrayLayer = std::max(srcRange.baseArrayLayer, dstRange.baseArrayLayer);

					if (srcRange.layerCount == VK_REMAINING_ARRAY_LAYERS) {
						if (dstRange.layerCount == VK_REMAINING_ARRAY_LAYERS) {
							match.range.layerCount = VK_REMAINING_ARRAY_LAYERS;
						} else {
							match.range.layerCount =
								srcRange.baseArrayLayer + srcRange.layerCount - match.range.baseArrayLayer;
						}
					} else if (dstRange.layerCount == VK_REMAINING_ARRAY_LAYERS) {
						match.range.layerCount =
							srcRange.baseArrayLayer + srcRange.layerCount - match.range.baseArrayLayer;
					} else {
						match.range.layerCount = std::min(srcRange.baseArrayLayer + srcRange.layerCount,
														  dstRange.baseArrayLayer + dstRange.layerCount) -
												 match.range.baseArrayLayer;
					}

					match.range.baseMipLevel = std::max(srcRange.baseMipLevel, dstRange.baseMipLevel);

					if (srcRange.levelCount == VK_REMAINING_MIP_LEVELS) {
						if (dstRange.levelCount == VK_REMAINING_MIP_LEVELS) {
							match.range.levelCount = VK_REMAINING_MIP_LEVELS;
						} else {
							match.range.levelCount =
								srcRange.baseMipLevel + srcRange.levelCount - match.range.baseMipLevel;
						}
					} else if (dstRange.levelCount == VK_REMAINING_MIP_LEVELS) {
						match.range.levelCount = srcRange.baseMipLevel + srcRange.levelCount - match.range.baseMipLevel;
					} else {
						match.range.levelCount = std::min(srcRange.baseMipLevel + srcRange.levelCount,
														  dstRange.baseMipLevel + dstRange.levelCount) -
												 match.range.baseMipLevel;
					}
					return match;
				}
			}
			if (nextModificationIterator == modifications.begin()) {
				break;
			}
			--nextModificationIterator;
		}
		return std::nullopt;
	}

	void QueueBarrierGenerator::emitBarrier(SlotmapHandle buffer, const BufferAccessMatch& match,
											const BufferSubresourceAccess& write, const BufferSubresourceAccess& read) {
		auto barrierIterator = std::find_if(m_nodeBarrierInfos[write.nodeIndex].bufferBarriers.begin(),
											m_nodeBarrierInfos[write.nodeIndex].bufferBarriers.end(),
											[buffer](const auto& barrier) { return barrier.buffer == buffer; });
		if (barrierIterator == m_nodeBarrierInfos[write.nodeIndex].bufferBarriers.end()) {
			m_nodeBarrierInfos[write.nodeIndex].bufferBarriers.push_back(
				{ .dstNodeIndex = read.nodeIndex,
				  .srcPipelineStageFlags = write.accessingPipelineStages,
				  .dstPipelineStageFlags = read.accessingPipelineStages,
				  .srcAccessFlags = write.access,
				  .dstAccessFlags = read.access,
				  .offset = match.offset,
				  .size = match.size,
				  .buffer = buffer });
		} else {
			auto& barrier = *barrierIterator;
			barrier.dstNodeIndex = std::min(barrier.dstNodeIndex, read.nodeIndex);
			barrier.dstPipelineStageFlags |= read.accessingPipelineStages;
			barrier.dstAccessFlags |= read.access;
			if (barrier.offset > match.offset) {
				barrier.offset = match.offset;
				if (barrier.size != VK_WHOLE_SIZE)
					barrier.size += barrier.offset - match.offset;
			}
			barrier.size = std::max(barrier.offset + barrier.size, match.offset + match.size) - barrier.offset;
		}
	}
	void QueueBarrierGenerator::emitBarrier(std::optional<SlotmapHandle> image, const ImageAccessMatch& match,
											const ImageSubresourceAccess& write, const ImageSubresourceAccess& read) {
		auto barrierIterator = std::find_if(m_nodeBarrierInfos[write.nodeIndex].imageBarriers.begin(),
											m_nodeBarrierInfos[write.nodeIndex].imageBarriers.end(),
											[image](const auto& barrier) { return barrier.image == image; });
		if (barrierIterator == m_nodeBarrierInfos[write.nodeIndex].imageBarriers.end()) {
			VkImageLayout newLayout = read.startLayout;
			if (newLayout == VK_IMAGE_LAYOUT_UNDEFINED) {
				newLayout = write.finishLayout;
			}
			m_nodeBarrierInfos[write.nodeIndex].imageBarriers.push_back(ImageFramegraphBarrier{
				.dstNodeIndex = read.nodeIndex,
				.srcPipelineStageFlags = write.accessingPipelineStages,
				.dstPipelineStageFlags = read.accessingPipelineStages,
				.srcAccessFlags = write.access,
				.dstAccessFlags = read.access,
				.subresourceRange = match.range,
				.beforeLayout = write.finishLayout,
				.afterLayout = newLayout,
				.image = image,
			});
		} else {
			auto& barrier = *barrierIterator;
			barrier.dstNodeIndex = std::min(barrier.dstNodeIndex, read.nodeIndex);
			barrier.dstPipelineStageFlags |= read.accessingPipelineStages;
			barrier.dstAccessFlags |= read.access;

			auto& dstRange = barrier.subresourceRange;
			dstRange.aspectMask |= match.range.aspectMask;

			if (dstRange.baseArrayLayer > match.range.baseArrayLayer) {
				dstRange.baseArrayLayer = match.range.baseArrayLayer;
				if (dstRange.layerCount != VK_REMAINING_ARRAY_LAYERS)
					dstRange.layerCount += dstRange.baseArrayLayer - match.range.baseArrayLayer;
			}
			dstRange.layerCount = std::max(dstRange.baseArrayLayer + dstRange.layerCount,
										   match.range.baseArrayLayer + match.range.layerCount) -
								  dstRange.baseArrayLayer;
			if (dstRange.baseMipLevel > match.range.baseMipLevel) {
				dstRange.baseMipLevel = match.range.baseMipLevel;
				if (dstRange.levelCount != VK_REMAINING_MIP_LEVELS)
					dstRange.levelCount += dstRange.baseMipLevel - match.range.baseMipLevel;
			}
			dstRange.levelCount = std::max(dstRange.baseMipLevel + dstRange.levelCount,
										   match.range.baseMipLevel + match.range.levelCount) -
								  dstRange.baseMipLevel;
		}
	}
} // namespace vanadium::graphics
