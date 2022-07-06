#include <Log.hpp>
#include <graphics/util/RangeAllocator.hpp>
#include <algorithm>

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

	std::optional<RangeAllocationResult> allocateFromRanges(SimpleVector<MemoryRange>& gapsOffsetSorted,
															SimpleVector<MemoryRange>& gapsSizeSorted,
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
			std::sort(gapsOffsetSorted.begin(), gapsOffsetSorted.end(), offsetComparator);
			std::sort(gapsSizeSorted.begin(), gapsSizeSorted.end(), sizeComparator);
		} else {
			gapsOffsetSorted.erase(offsetIterator);
			auto sizeIterator = std::lower_bound(gapsSizeSorted.begin(), gapsSizeSorted.end(), range, sizeComparator);
			gapsSizeSorted.erase(sizeIterator);
		}

		return result;
	}

	void freeToRanges(SimpleVector<MemoryRange>& gapsOffsetSorted, SimpleVector<MemoryRange>& gapsSizeSorted,
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

	void mergeFreeAreas(SimpleVector<MemoryRange>& gapsOffsetSorted, SimpleVector<MemoryRange>& gapsSizeSorted) {
		auto sizeComparator = [](const MemoryRange& one, const MemoryRange& other) { return one.size < other.size; };
		
		if (gapsOffsetSorted.empty())
			return;
		for (size_t i = 0; i < gapsOffsetSorted.size() - 1; ++i) {
			auto& area = gapsOffsetSorted[i];
			auto& nextArea = gapsOffsetSorted[i + 1];

			if (area.offset + area.size == nextArea.offset) {
				area.size += nextArea.size;
				auto areaIterator = std::find_if(gapsSizeSorted.begin(), gapsSizeSorted.end(),
												 [area](const auto& gap) { return area.offset == gap.offset; });
				assertFatal(areaIterator != gapsSizeSorted.end(), "RangeAllocator inconsistency!");
				areaIterator->size += nextArea.size;

				auto nextAreaIterator =
					std::find_if(gapsSizeSorted.begin(), gapsSizeSorted.end(),
								 [nextArea](const auto& gap) { return nextArea.offset == gap.offset; });
				assertFatal(nextAreaIterator != gapsSizeSorted.end(), "RangeAllocator inconsistency!");
				gapsOffsetSorted.erase(gapsOffsetSorted.begin() + i + 1);
				gapsSizeSorted.erase(nextAreaIterator);

				std::sort(gapsSizeSorted.begin(), gapsSizeSorted.end(), sizeComparator);
				--i;
			}
		}
	}
} // namespace vanadium::graphics