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
#define VK_NO_PROTOTYPES
#include <graphics/helper/ErrorHelper.hpp>
#include <vulkan/vulkan.h>

template <typename HandleType, typename Enumerant, typename... AdditionalParams>
using AdditionalParameterEnumerationPFN = VkResult(VKAPI_PTR*)(HandleType type, AdditionalParams... params,
															   uint32_t* count, Enumerant* pEnumerants);
template <typename HandleType, typename Enumerant>
using EnumerationPFN = VkResult(VKAPI_PTR*)(HandleType type, uint32_t* count, Enumerant* pEnumerants);

template <typename HandleType, typename Enumerant, typename... AdditionalParams>
std::vector<Enumerant> enumerate(
	HandleType handle, AdditionalParams... params,
	AdditionalParameterEnumerationPFN<HandleType, Enumerant, AdditionalParams...> pfnEnumerator) {
	std::vector<Enumerant> values;
	uint32_t valueCount;
	VkResult enumerationResult;

	do {
		verifyResult(pfnEnumerator(handle, params..., &valueCount, nullptr));
		values.resize(valueCount);
		enumerationResult = pfnEnumerator(handle, params..., &valueCount, values.data());
	} while (enumerationResult == VK_INCOMPLETE);

	verifyResult(enumerationResult);
	return values;
}

template <typename HandleType, typename Enumerant>
std::vector<Enumerant> enumerate(HandleType handle, EnumerationPFN<HandleType, Enumerant> pfnEnumerator) {
	std::vector<Enumerant> values;
	uint32_t valueCount;
	VkResult enumerationResult;

	do {
		verifyResult(pfnEnumerator(handle, &valueCount, nullptr));
		values = std::vector<Enumerant>(valueCount);
		enumerationResult = pfnEnumerator(handle, &valueCount, values.data());
	} while (enumerationResult == VK_INCOMPLETE);

	verifyResult(enumerationResult);
	return values;
}
