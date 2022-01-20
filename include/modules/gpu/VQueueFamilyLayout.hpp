#pragma once

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

namespace gpu {
	struct VQueueFamilyLayout {
		VkQueueFlags queueCapabilityFlags;
		bool shouldSupportPresent;
		size_t queueCount;
	};
} // namespace gpu