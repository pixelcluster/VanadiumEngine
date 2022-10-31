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

#include <Log.hpp>
#include <graphics/backend/vulkan/Enumeration.hpp>
#include <graphics/backend/vulkan/Instance.hpp>
#include <graphics/backend/vulkan/Device.hpp>

#define VK_NO_PROTOTYPES
#include <volk.h>

namespace vanadium::graphics::rhi::vulkan {

	const char* platformSurfaceExtensionName(const std::vector<VkExtensionProperties>& instanceExtensions) {
#ifdef VK_USE_PLATFORM_WIN32_KHR
		vanadium::logInfo("Using Win32.");
		return VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
#else
		if (glfwGetPlatform() == GLFW_PLATFORM_WAYLAND) {
			logInfo(SubsystemID::RHI, "Using Wayland.");
			return VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME;
		}
		logInfo(SubsystemID::RHI, "Using X11.");
		if (std::find_if(instanceExtensions.begin(), instanceExtensions.end(), [](const auto& properties) {
				return !strcmp(properties.extensionName, VK_KHR_XCB_SURFACE_EXTENSION_NAME);
			}) != instanceExtensions.end()) {
			return VK_KHR_XCB_SURFACE_EXTENSION_NAME;
		} else {
			return VK_KHR_XLIB_SURFACE_EXTENSION_NAME;
		}
#endif
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL debugLog(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
											VkDebugUtilsMessageTypeFlagsEXT messageTypes,
											const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
											void* pUserData) {
		switch (messageSeverity) {
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
				logInfo(SubsystemID::RHI, pCallbackData->pMessage);
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
				logWarning(SubsystemID::RHI, pCallbackData->pMessage);
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
				logError(SubsystemID::RHI, pCallbackData->pMessage);
				break;
			default:
				break;
		}
		return VK_FALSE;
	}

	Instance* Instance::create(const std::string_view& appName, uint32_t appVersion) {
		return new Instance(appName, appVersion);
	}

	Device* Instance::createDevice(const PhysicalDevice& device) {
		return new Device(device);
	}

	Surface* Instance::createSurface(windowing::WindowInterface* interface) { return nullptr; }

	Instance::Instance(const std::string_view& appName, uint32_t appVersion) {

		VkApplicationInfo appInfo = { .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
									  .pApplicationName = appName.data(),
									  .applicationVersion = appVersion,
									  .pEngineName = "Vanadium",
									  .engineVersion = 1,
									  .apiVersion = VK_API_VERSION_1_3 };

		std::vector<const char*> instanceExtensionNames;
		instanceExtensionNames.reserve(4);

		std::vector<VkExtensionProperties> availableInstanceExtensions =
			enumerate<const char*, VkExtensionProperties>(nullptr, vkEnumerateInstanceExtensionProperties);

		instanceExtensionNames.push_back(platformSurfaceExtensionName(availableInstanceExtensions));
		instanceExtensionNames.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
		for (auto& extensionProperties : availableInstanceExtensions) {
			if (!strcmp(extensionProperties.extensionName, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME)) {
				instanceExtensionNames.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
			}
		}

		VkInstanceCreateInfo instanceCreateInfo = { .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
													.pApplicationInfo = &appInfo };
		VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
			.messageSeverity =
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT,
			.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
						   VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
						   VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,
			.pfnUserCallback = debugLog
		};

		if constexpr (vanadiumGPUDebug) {
			instanceCreateInfo.pNext = &debugUtilsMessengerCreateInfo;
			instanceExtensionNames.push_back("VK_EXT_debug_utils");
		}

		instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensionNames.size()),
		instanceCreateInfo.ppEnabledExtensionNames = instanceExtensionNames.data();

		verifyResult(vkCreateInstance(&instanceCreateInfo, nullptr, &m_instance));

		if constexpr (vanadiumGPUDebug) {
			verifyResult(vkCreateDebugUtilsMessengerEXT(m_instance, &debugUtilsMessengerCreateInfo, NULL, &m_debugMessenger));
		}
	}
} // namespace vanadium::graphics::rhi::vulkan
