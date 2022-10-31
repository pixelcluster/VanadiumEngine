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
#pragma once

#include <vector>
#include <optional>

namespace vanadium::graphics {
	struct MemoryRange {
		size_t offset;
		size_t size;
	};

	struct RangeAllocationResult {
		MemoryRange allocationRange;
		MemoryRange usableRange;
	};

	size_t roundUpAligned(size_t n, size_t alignment);
	size_t alignmentMargin(size_t n, size_t alignment);

	std::optional<RangeAllocationResult> allocateFromRanges(std::vector<MemoryRange>& gapsOffsetSorted,
															std::vector<MemoryRange>& gapsSizeSorted,
															size_t alignment, size_t size);

	void freeToRanges(std::vector<MemoryRange>& gapsOffsetSorted, std::vector<MemoryRange>& gapsSizeSorted,
					  size_t offset, size_t size);
	void mergeFreeAreas(std::vector<MemoryRange>& gapsOffsetSorted, std::vector<MemoryRange>& gapsSizeSorted);
} // namespace vanadium::graphics
