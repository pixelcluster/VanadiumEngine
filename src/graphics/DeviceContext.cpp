#include <Debug.hpp>
#include <cstring>
#include <graphics/DeviceContext.hpp>
#include <graphics/helper/EnumerationHelper.hpp>
#include <graphics/helper/ErrorHelper.hpp>
#include <iostream>
#include <optional>
#include <vector>
#include <volk.h>

const char* platformSurfaceExtensionName() {
#if defined(VK_USE_PLATFORM_WIN32_KHR)
	return VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
#elif defined(VK_USE_PLATFORM_XCB_KHR)
	return VK_KHR_XCB_SURFACE_EXTENSION_NAME;
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
	return VK_KHR_XLIB_SURFACE_EXTENSION_NAME;
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
	return VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME;
#endif
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugLog(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
										VkDebugUtilsMessageTypeFlagsEXT messageTypes,
										const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
	std::cout << pCallbackData->pMessage << "\n";
	return VK_FALSE;
}

namespace vanadium::graphics {
	DeviceContext::DeviceContext(const std::string_view& appName, uint32_t appVersion, WindowSurface& windowSurface) {
		verifyResult(volkInitialize());

		VkApplicationInfo appInfo = { .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
									  .applicationVersion = appVersion,
									  .pApplicationName = appName.data(),
									  .engineVersion = 1,
									  .pEngineName = "Vanadium",
									  .apiVersion = VK_API_VERSION_1_0 };

		std::vector<const char*> instanceExtensionNames;
		instanceExtensionNames.reserve(4);
		instanceExtensionNames.push_back(platformSurfaceExtensionName());
		instanceExtensionNames.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

		std::vector<VkExtensionProperties> availableInstanceExtensions =
			enumerate<const char*, VkExtensionProperties>(nullptr, vkEnumerateInstanceExtensionProperties);
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
			vkCreateDebugUtilsMessengerEXT(m_instance, &debugUtilsMessengerCreateInfo, nullptr, &m_debugMessenger);
		}

		windowSurface.create(m_instance);

		std::vector<VkPhysicalDevice> physicalDevices =
			enumerate<VkInstance, VkPhysicalDevice>(m_instance, vkEnumeratePhysicalDevices);

		std::optional<VkPhysicalDevice> chosenDevice = std::nullopt;
		uint32_t chosenQueueFamilyIndex = -1;
		for (auto& device : physicalDevices) {
			uint32_t propertyCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(device, &propertyCount, nullptr);
			auto queueFamilyProperties = std::vector<VkQueueFamilyProperties>(propertyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(device, &propertyCount, queueFamilyProperties.data());

			uint32_t queueFamilyIndex = 0;
			for (auto& properties : queueFamilyProperties) {
				VkBool32 surfaceSupport = VK_FALSE;
				vkGetPhysicalDeviceSurfaceSupportKHR(device, queueFamilyIndex, windowSurface.surface(),
													 &surfaceSupport);
				if ((properties.queueFlags & VK_QUEUE_GRAPHICS_BIT) && surfaceSupport) {
					chosenDevice = device;
					chosenQueueFamilyIndex = queueFamilyIndex;
					break;
				}

				++queueFamilyIndex;
			}

			if (chosenDevice.has_value()) {
				break;
			}
		}

		if (!chosenDevice.has_value()) {
			std::exit(-5);
		}

		m_physicalDevice = chosenDevice.value();

		windowSurface.setPhysicalDevice(m_physicalDevice);

		std::vector<const char*> deviceExtensionNames = { "VK_KHR_swapchain" };

		std::vector<VkExtensionProperties> availableDeviceExtensions =
			enumerate<VkPhysicalDevice, VkExtensionProperties, const char*>(m_physicalDevice, nullptr,
																			vkEnumerateDeviceExtensionProperties);

		for (auto& extension : availableDeviceExtensions) {
			if (!strcmp(extension.extensionName, VK_EXT_MEMORY_BUDGET_EXTENSION_NAME)) {
				deviceExtensionNames.push_back(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME);
				m_capabilities.memoryBudget = true;
			}
			if (!strcmp(extension.extensionName, VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME)) {
				deviceExtensionNames.push_back(VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME);
				m_capabilities.memoryPriority = true;
			}
		}

		float priority = 1.0f;
		VkDeviceQueueCreateInfo queueCreateInfo = { .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
													.queueFamilyIndex = chosenQueueFamilyIndex,
													.queueCount = 1,
													.pQueuePriorities = &priority };

		VkDeviceCreateInfo deviceCreateInfo = { .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
												.queueCreateInfoCount = 1,
												.pQueueCreateInfos = &queueCreateInfo,
												.enabledExtensionCount =
													static_cast<uint32_t>(deviceExtensionNames.size()),
												.ppEnabledExtensionNames = deviceExtensionNames.data() };
		verifyResult(vkCreateDevice(chosenDevice.value(), &deviceCreateInfo, nullptr, &m_device));

		volkLoadDevice(m_device);

		m_physicalDevice = chosenDevice.value();
		std::vector<VkSurfaceFormatKHR> surfaceFormats = enumerate<VkPhysicalDevice, VkSurfaceFormatKHR, VkSurfaceKHR>(
			m_physicalDevice, m_surface, vkGetPhysicalDeviceSurfaceFormatsKHR);

		if (std::find_if(surfaceFormats.begin(), surfaceFormats.end(), [](const auto& format) {
				return format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR &&
					   format.format == VK_FORMAT_B8G8R8A8_SRGB;
			}) == surfaceFormats.end()) {
			std::exit(-6);
		}

		vkGetDeviceQueue(m_device, chosenQueueFamilyIndex, 0, &m_graphicsQueue);

		m_graphicsQueueFamilyIndex = chosenQueueFamilyIndex;

		VkFenceCreateInfo fenceCreateInfo = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
											  .flags = VK_FENCE_CREATE_SIGNALED_BIT };

		VkSemaphoreCreateInfo semaphoreCreateInfo = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

		VkCommandPoolCreateInfo poolCreateInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
												   .queueFamilyIndex = m_graphicsQueueFamilyIndex };

		for (size_t i = 0; i < frameInFlightCount; ++i) {
			verifyResult(vkCreateFence(m_device, &fenceCreateInfo, nullptr, &m_frameCompletionFences[i]));
			verifyResult(vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &m_swapchainImageSemaphores[i]));

			verifyResult(vkCreateCommandPool(m_device, &poolCreateInfo, nullptr, &m_frameCommandPools[i]));
		}
	}
} // namespace vanadium::graphics