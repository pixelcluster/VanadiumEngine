#pragma once

#define VK_NO_PROTOTYPES
#include <graphics/helper/ErrorHelper.hpp>
#include <string>
#include <volk.h>
#include <vulkan/vulkan.h>

template <typename T> inline void setObjectName(VkDevice device, VkObjectType type, T handle, const std::string& name) {
	VkDebugUtilsObjectNameInfoEXT nameInfo = { .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
											   .objectType = type,
											   .objectHandle = reinterpret_cast<uint64_t>(handle),
											   .pObjectName = name.c_str() };
	verifyResult(vkSetDebugUtilsObjectNameEXT(device, &nameInfo));
}