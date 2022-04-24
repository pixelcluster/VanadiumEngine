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

	void reorderOffsetArea(std::vector<MemoryRange>& gapsOffsetSorted, std::vector<MemoryRange>& gapsSizeSorted,
						   size_t index);
	void reorderSizeArea(std::vector<MemoryRange>& gapsOffsetSorted, std::vector<MemoryRange>& gapsSizeSorted,
						 size_t index);

	std::optional<RangeAllocationResult> allocateFromRanges(std::vector<MemoryRange>& gapsOffsetSorted,
															std::vector<MemoryRange>& gapsSizeSorted,
															VkDeviceSize alignment, VkDeviceSize size);

	void freeToRanges(std::vector<MemoryRange>& gapsOffsetSorted, std::vector<MemoryRange>& gapsSizeSorted,
					  VkDeviceSize offset, VkDeviceSize size);
	void mergeFreeAreas(std::vector<MemoryRange>& gapsOffsetSorted, std::vector<MemoryRange>& gapsSizeSorted);
} // namespace vanadium::graphics
