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
#include <Log.hpp>
#include <algorithm>
#include <graphics/util/RangeAllocator.hpp>

namespace vanadium::graphics {
	size_t roundUpAligned(size_t n, size_t alignment) { return n + alignmentMargin(n, alignment); }
	size_t alignmentMargin(size_t n, size_t alignment) {
		if (alignment) {
			size_t remainder = n % alignment;
			if (remainder) {
				return alignment - remainder;
			}
		}
		return 0;
	}

	std::optional<RangeAllocationResult> allocateFromRanges(std::vector<MemoryRange>& gapsOffsetSorted,
															std::vector<MemoryRange>& gapsSizeSorted, size_t alignment,
															size_t size) {
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

		size_t margin = alignmentMargin(gapsSizeSorted[allocationIndex].offset, alignment);
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

	void freeToRanges(std::vector<MemoryRange>& gapsOffsetSorted, std::vector<MemoryRange>& gapsSizeSorted,
					  size_t offset, size_t size) {
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
				auto areaIterator = std::find_if(gapsSizeSorted.begin(), gapsSizeSorted.end(),
												 [area](const auto& gap) { return area.offset == gap.offset; });
				assertFatal(areaIterator != gapsSizeSorted.end(), SubsystemID::Unknown, "RangeAllocator inconsistency!");
				areaIterator->size += nextArea.size;

				auto nextAreaIterator =
					std::find_if(gapsSizeSorted.begin(), gapsSizeSorted.end(),
								 [nextArea](const auto& gap) { return nextArea.offset == gap.offset; });
				assertFatal(nextAreaIterator != gapsSizeSorted.end(), SubsystemID::Unknown, "RangeAllocator inconsistency!");
				gapsOffsetSorted.erase(gapsOffsetSorted.begin() + i + 1);
				gapsSizeSorted.erase(nextAreaIterator);

				std::sort(gapsSizeSorted.begin(), gapsSizeSorted.end(), sizeComparator);
				--i;
			}
		}
	}
} // namespace vanadium::graphics
