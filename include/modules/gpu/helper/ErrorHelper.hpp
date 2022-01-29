#pragma once

#define VK_NO_PROTOTYPES
#include <cassert>
#include <vulkan/vulkan.h>

inline void verifyResult(VkResult result) { assert(result >= 0); }