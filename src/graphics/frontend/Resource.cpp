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
#include <graphics/frontend/Resource.hpp>

namespace vanadium::graphics::rhi {
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

	void mergeFreeAreas(std::vector<MemoryRange>& m_gapsOffsetSorted, std::vector<MemoryRange>& m_gapsSizeSorted) {
		auto sizeComparator = [](const MemoryRange& one, const MemoryRange& other) { return one.size < other.size; };

		if (m_gapsOffsetSorted.empty())
			return;
		for (size_t i = 0; i < m_gapsOffsetSorted.size() - 1; ++i) {
			auto& area = m_gapsOffsetSorted[i];
			auto& nextArea = m_gapsOffsetSorted[i + 1];

			if (area.offset + area.size == nextArea.offset) {
				area.size += nextArea.size;
				auto areaIterator = std::find_if(m_gapsSizeSorted.begin(), m_gapsSizeSorted.end(),
												 [area](const auto& gap) { return area.offset == gap.offset; });
				assertFatal(areaIterator != m_gapsSizeSorted.end(), SubsystemID::Unknown, "RangeAllocator inconsistency!");
				areaIterator->size += nextArea.size;

				auto nextAreaIterator =
					std::find_if(m_gapsSizeSorted.begin(), m_gapsSizeSorted.end(),
								 [nextArea](const auto& gap) { return nextArea.offset == gap.offset; });
				assertFatal(nextAreaIterator != m_gapsSizeSorted.end(), SubsystemID::Unknown, "RangeAllocator inconsistency!");
				m_gapsOffsetSorted.erase(m_gapsOffsetSorted.begin() + i + 1);
				m_gapsSizeSorted.erase(nextAreaIterator);

				std::sort(m_gapsSizeSorted.begin(), m_gapsSizeSorted.end(), sizeComparator);
				--i;
			}
		}
	}

	std::optional<RangeAllocationResult> MemoryBlock::reserveRange(size_t size, size_t alignment) {
		auto offsetComparator = [](const MemoryRange& one, const MemoryRange& other) {
			return one.offset < other.offset;
		};
		auto sizeComparator = [](const MemoryRange& one, const MemoryRange& other) { return one.size < other.size; };

		// seek for smallest available block
		size_t allocationIndex = m_gapsSizeSorted.size() - 1;
		while (allocationIndex < m_gapsSizeSorted.size() &&
			   m_gapsSizeSorted[allocationIndex].size >=
				   size + alignmentMargin(m_gapsSizeSorted[allocationIndex].offset, alignment)) {
			--allocationIndex;
		}
		++allocationIndex;
		if (allocationIndex == m_gapsSizeSorted.size()) {
			return std::nullopt;
		}

		size_t margin = alignmentMargin(m_gapsSizeSorted[allocationIndex].offset, alignment);
		auto& usedRange = m_gapsSizeSorted[allocationIndex];

		RangeAllocationResult result = { .allocationRange = { .offset = usedRange.offset, .size = size + margin },
										 .usableRange = { .offset = usedRange.offset + margin, .size = size } };

		auto& range = m_gapsSizeSorted[allocationIndex];
		auto offsetIterator =
			std::lower_bound(m_gapsOffsetSorted.begin(), m_gapsOffsetSorted.end(), range, offsetComparator);

		if (result.allocationRange.size != range.size) {
			// make unused part of block another free range
			range.offset += result.allocationRange.size;
			range.size -= result.allocationRange.size;
			offsetIterator->offset += result.allocationRange.size;
			offsetIterator->size -= result.allocationRange.size;
			std::sort(m_gapsOffsetSorted.begin(), m_gapsOffsetSorted.end(), offsetComparator);
			std::sort(m_gapsSizeSorted.begin(), m_gapsSizeSorted.end(), sizeComparator);
		} else {
			m_gapsOffsetSorted.erase(offsetIterator);
			auto sizeIterator =
				std::lower_bound(m_gapsSizeSorted.begin(), m_gapsSizeSorted.end(), range, sizeComparator);
			m_gapsSizeSorted.erase(sizeIterator);
		}

		return result;
	}

	void MemoryBlock::freeRange(const MemoryRange& allocatedRange) {
		auto offsetComparator = [](const MemoryRange& one, const MemoryRange& other) {
			return one.offset < other.offset;
		};
		auto sizeComparator = [](const MemoryRange& one, const MemoryRange& other) { return one.size < other.size; };

		if (m_gapsOffsetSorted.empty()) {
			m_gapsOffsetSorted.push_back(allocatedRange);
			m_gapsSizeSorted.push_back(allocatedRange);
		}

		auto offsetInsertIterator =
			std::lower_bound(m_gapsOffsetSorted.begin(), m_gapsOffsetSorted.end(), allocatedRange, offsetComparator);

		if (offsetInsertIterator == m_gapsOffsetSorted.end()) {
			m_gapsOffsetSorted.push_back(allocatedRange);
		} else {
			m_gapsOffsetSorted.insert(offsetInsertIterator, allocatedRange);
		}

		auto sizeInsertIterator =
			std::lower_bound(m_gapsSizeSorted.begin(), m_gapsSizeSorted.end(), allocatedRange, sizeComparator);

		if (sizeInsertIterator == m_gapsSizeSorted.end())
			m_gapsSizeSorted.push_back(allocatedRange);
		else
			m_gapsSizeSorted.insert(sizeInsertIterator, allocatedRange);

		mergeFreeAreas(m_gapsOffsetSorted, m_gapsSizeSorted);
	}
} // namespace vanadium::graphics::rhi
