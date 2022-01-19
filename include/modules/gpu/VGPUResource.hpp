#pragma once

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

namespace gpu {
	struct VGPUResource {
		VkObjectType type;
		uint64_t handle;

		VkImageLayout outputLayout;
	};
}