#pragma once

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <util/Vector.hpp>
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

	std::optional<RangeAllocationResult> allocateFromRanges(SimpleVector<MemoryRange>& gapsOffsetSorted,
															SimpleVector<MemoryRange>& gapsSizeSorted,
															VkDeviceSize alignment, VkDeviceSize size);

	void freeToRanges(SimpleVector<MemoryRange>& gapsOffsetSorted, SimpleVector<MemoryRange>& gapsSizeSorted,
					  VkDeviceSize offset, VkDeviceSize size);
	void mergeFreeAreas(SimpleVector<MemoryRange>& gapsOffsetSorted, SimpleVector<MemoryRange>& gapsSizeSorted);
} // namespace vanadium::graphics
