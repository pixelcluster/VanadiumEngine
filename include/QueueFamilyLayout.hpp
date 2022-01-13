#pragma once

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

struct QueueFamilyLayout {
	VkQueueFlags queueCapabilityFlags;
	bool shouldSupportPresent;
	size_t queueCount;
};