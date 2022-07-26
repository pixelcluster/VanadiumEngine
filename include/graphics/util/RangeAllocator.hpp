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

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <vector>
#include <optional>

namespace vanadium::graphics {
	struct MemoryRange {
		VkDeviceSize offset;
		VkDeviceSize size;
	};

	struct RangeAllocationResult {
		MemoryRange allocationRange;
		MemoryRange usableRange;
	};

	VkDeviceSize roundUpAligned(VkDeviceSize n, VkDeviceSize alignment);
	VkDeviceSize alignmentMargin(VkDeviceSize n, VkDeviceSize alignment);

	std::optional<RangeAllocationResult> allocateFromRanges(std::vector<MemoryRange>& gapsOffsetSorted,
															std::vector<MemoryRange>& gapsSizeSorted,
															VkDeviceSize alignment, VkDeviceSize size);

	void freeToRanges(std::vector<MemoryRange>& gapsOffsetSorted, std::vector<MemoryRange>& gapsSizeSorted,
					  VkDeviceSize offset, VkDeviceSize size);
	void mergeFreeAreas(std::vector<MemoryRange>& gapsOffsetSorted, std::vector<MemoryRange>& gapsSizeSorted);
} // namespace vanadium::graphics
