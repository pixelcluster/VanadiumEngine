#include <graphics/util/RangeAllocator.hpp>

namespace vanadium::graphics {
	VkDeviceSize roundUpAligned(VkDeviceSize n, VkDeviceSize alignment) { return n + alignmentMargin(n, alignment); }
	VkDeviceSize alignmentMargin(VkDeviceSize n, VkDeviceSize alignment) {
		if (alignment) {
			VkDeviceSize remainder = n % alignment;
			if (remainder) {
				return alignment - remainder;
			}
		}
		return 0;
	}

	void reorderOffsetArea(std::vector<MemoryRange>& gapsOffsetSorted, std::vector<MemoryRange>& gapsSizeSorted,
						   size_t index) {
		auto offsetComparator = [](const MemoryRange& one, const MemoryRange& other) {
			return one.offset < other.offset;
		};
		auto range = gapsOffsetSorted[index];
		auto reinsertIterator =
			std::lower_bound(gapsOffsetSorted.begin(), gapsOffsetSorted.end(), range, offsetComparator);

		if (reinsertIterator == gapsOffsetSorted.end() || reinsertIterator == gapsOffsetSorted.end() - 1) {
			gapsOffsetSorted.erase(gapsOffsetSorted.begin() + index);
			gapsOffsetSorted.push_back(range);
		} else {
			for (auto iterator = gapsOffsetSorted.begin() + index; iterator != reinsertIterator; ++iterator) {
				*iterator = std::move(*(iterator + 1));
			}
			*reinsertIterator = range;
		}
	}

	void reorderSizeArea(std::vector<MemoryRange>& gapsOffsetSorted, std::vector<MemoryRange>& gapsSizeSorted,
						 size_t index) {
		auto sizeComparator = [](const MemoryRange& one, const MemoryRange& other) { return one.size < other.size; };

		auto range = gapsSizeSorted[index];
		auto reinsertIterator = std::lower_bound(gapsSizeSorted.begin(), gapsSizeSorted.end(), range, sizeComparator);

		if (reinsertIterator == gapsSizeSorted.end() || reinsertIterator == gapsSizeSorted.end() - 1) {
			gapsSizeSorted.erase(gapsSizeSorted.begin() + index);
			gapsSizeSorted.push_back(range);
		} else {
			for (auto iterator = gapsSizeSorted.begin() + index; iterator != reinsertIterator; ++iterator) {
				*iterator = std::move(*(iterator + 1));
			}
			*reinsertIterator = range;
		}
	}

	std::optional<RangeAllocationResult> allocateFromRanges(std::vector<MemoryRange>& gapsOffsetSorted,
															std::vector<MemoryRange>& gapsSizeSorted,
															VkDeviceSize alignment, VkDeviceSize size) {
		auto offsetComparator = [](const MemoryRange& one, const MemoryRange& other) {
			return one.offset < other.offset;
		};
		auto sizeComparator = [](const MemoryRange& one, const MemoryRange& other) { return one.size < other.size; };

		// seek for smallest available block
		size_t allocationIndex = gapsSizeSorted.size() - 1;
		while (allocationIndex < gapsSizeSorted.size() &&
			   gapsSizeSorted[allocationIndex].size >=
				   size + alignmentMargin(gapsSizeSorted[allocationIndex].offset, alignment)) {
			--allocationIndex;
		}
		++allocationIndex;
		if (allocationIndex == gapsSizeSorted.size()) {
			return std::nullopt;
		}

		VkDeviceSize margin = alignmentMargin(gapsSizeSorted[allocationIndex].offset, alignment);
		auto& usedRange = gapsSizeSorted[allocationIndex];

		RangeAllocationResult result = { .allocationRange = { .offset = usedRange.offset, .size = size + margin },
										 .usableRange = { .offset = usedRange.offset + margin, .size = size } };

		auto& range = gapsSizeSorted[allocationIndex];
		auto offsetIterator =
			std::lower_bound(gapsOffsetSorted.begin(), gapsOffsetSorted.end(), range, offsetComparator);

		if (result.allocationRange.size != range.size) {
			// make unused part of block another free range
			range.offset += result.allocationRange.size;
			range.size -= result.allocationRange.size;
			offsetIterator->offset += result.allocationRange.size;
			offsetIterator->size -= result.allocationRange.size;
			reorderOffsetArea(gapsOffsetSorted, gapsSizeSorted, allocationIndex);
			reorderSizeArea(gapsOffsetSorted, gapsSizeSorted, allocationIndex);
		} else {
			gapsOffsetSorted.erase(offsetIterator);
			auto sizeIterator = std::lower_bound(gapsSizeSorted.begin(), gapsSizeSorted.end(), range, sizeComparator);
			gapsSizeSorted.erase(sizeIterator);
		}

		return result;
	}

	void freeToRanges(std::vector<MemoryRange>& gapsOffsetSorted, std::vector<MemoryRange>& gapsSizeSorted,
					  VkDeviceSize offset, VkDeviceSize size) {
		auto offsetComparator = [](const MemoryRange& one, const MemoryRange& other) {
			return one.offset < other.offset;
		};
		auto sizeComparator = [](const MemoryRange& one, const MemoryRange& other) { return one.size < other.size; };

		if (gapsOffsetSorted.empty()) {
			MemoryRange range = { .offset = offset, .size = size };
			gapsOffsetSorted.push_back(range);
			gapsSizeSorted.push_back(range);
		}

		MemoryRange range = { .offset = offset, .size = size };

		auto offsetInsertIterator =
			std::lower_bound(gapsOffsetSorted.begin(), gapsOffsetSorted.end(), range, offsetComparator);

		if (offsetInsertIterator == gapsOffsetSorted.end()) {
			gapsOffsetSorted.push_back(range);
		} else {
			gapsOffsetSorted.insert(offsetInsertIterator, range);
		}

		auto sizeInsertIterator = std::lower_bound(gapsSizeSorted.begin(), gapsSizeSorted.end(), range, sizeComparator);

		if (sizeInsertIterator == gapsSizeSorted.end())
			gapsSizeSorted.push_back(range);
		else
			gapsSizeSorted.insert(sizeInsertIterator, range);

		mergeFreeAreas(gapsOffsetSorted, gapsSizeSorted);
	}

	void mergeFreeAreas(std::vector<MemoryRange>& gapsOffsetSorted, std::vector<MemoryRange>& gapsSizeSorted) {
		auto sizeComparator = [](const MemoryRange& one, const MemoryRange& other) { return one.size < other.size; };

		if (gapsOffsetSorted.empty())
			return;
		for (size_t i = 0; i < gapsOffsetSorted.size() - 1; ++i) {
			auto& area = gapsOffsetSorted[i];
			auto& nextArea = gapsOffsetSorted[i + 1];

			if (area.offset + area.size == nextArea.offset) {
				area.size += nextArea.size;
				gapsOffsetSorted.erase(gapsOffsetSorted.begin() + i + 1);
				reorderSizeArea(gapsOffsetSorted, gapsSizeSorted, i);
				--i;
			}
		}
	}
} // namespace vanadium::graphics