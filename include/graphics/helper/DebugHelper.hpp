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
