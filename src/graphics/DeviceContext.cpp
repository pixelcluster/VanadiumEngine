#include <Debug.hpp>
#include <cstring>
#include <graphics/DeviceContext.hpp>
#include <graphics/helper/EnumerationHelper.hpp>
#include <graphics/helper/ErrorHelper.hpp>
#include <iostream>
#include <optional>
#include <vector>
#include <volk.h>
#include <Log.hpp>

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
									  .pApplicationName = appName.data(),
									  .applicationVersion = appVersion,
									  .pEngineName = "Vanadium",
									  .engineVersion = 1,
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

		windowSurface.create(m_instance, frameInFlightCount);

		std::vector<VkPhysicalDevice> physicalDevices =
			enumerate<VkInstance, VkPhysicalDevice>(m_instance, vkEnumeratePhysicalDevices);

		std::optional<VkPhysicalDevice> chosenDevice = std::nullopt;
		uint32_t chosenGraphicsQueueFamilyIndex = -1U;

		uint32_t chosenTransferQueueFamilyIndex = -1U;
		for (auto& device : physicalDevices) {
			uint32_t propertyCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(device, &propertyCount, nullptr);
			auto queueFamilyProperties = std::vector<VkQueueFamilyProperties>(propertyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(device, &propertyCount, queueFamilyProperties.data());

			uint32_t unrelatedGraphicsFlags = -1U;
			uint32_t unrelatedTransferFlags = -1U;

			uint32_t queueFamilyIndex = 0;
			for (auto& properties : queueFamilyProperties) {
				uint32_t currentUnrelatedGraphicsFlags =
					std::popcount(properties.queueFlags & ~(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT));
				uint32_t currentUnrelatedTransferFlags =
					std::popcount(properties.queueFlags & ~(VK_QUEUE_TRANSFER_BIT));

				if ((properties.queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
					windowSurface.supportsPresent(device, queueFamilyIndex) &&
					currentUnrelatedGraphicsFlags < unrelatedGraphicsFlags) {
					chosenGraphicsQueueFamilyIndex = queueFamilyIndex;
					unrelatedGraphicsFlags = currentUnrelatedGraphicsFlags;
				}
				if ((properties.queueFlags & VK_QUEUE_TRANSFER_BIT) &&
					currentUnrelatedTransferFlags < unrelatedTransferFlags) {
					chosenTransferQueueFamilyIndex = queueFamilyIndex;
					unrelatedTransferFlags = currentUnrelatedTransferFlags;
				}

				++queueFamilyIndex;
			}
			if (chosenGraphicsQueueFamilyIndex != -1U && chosenTransferQueueFamilyIndex != -1U) {
				if (queueFamilyProperties[chosenTransferQueueFamilyIndex].queueFlags &
					(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) {
						logWarning("DeviceContext: Didn't find a transfer-only queue family, using a general purpose one.");
				}

				chosenDevice = device;
				break;
			}
		}

		if (!chosenDevice.has_value()) {
			std::exit(-5);
		}

		m_physicalDevice = chosenDevice.value();

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

		float graphicsPriority = 1.0f;
		float transferPriority = 0.2f;
		VkDeviceQueueCreateInfo queueCreateInfos[2] = { { .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
														  .queueFamilyIndex = chosenGraphicsQueueFamilyIndex,
														  .queueCount = 1,
														  .pQueuePriorities = &graphicsPriority },
														{ .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
														  .queueFamilyIndex = chosenTransferQueueFamilyIndex,
														  .queueCount = 1,
														  .pQueuePriorities = &transferPriority } };

		VkDeviceCreateInfo deviceCreateInfo = { .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
												.queueCreateInfoCount = 2,
												.pQueueCreateInfos = queueCreateInfos,
												.enabledExtensionCount =
													static_cast<uint32_t>(deviceExtensionNames.size()),
												.ppEnabledExtensionNames = deviceExtensionNames.data() };
		verifyResult(vkCreateDevice(chosenDevice.value(), &deviceCreateInfo, nullptr, &m_device));

		volkLoadDevice(m_device);

		m_physicalDevice = chosenDevice.value();

		vkGetDeviceQueue(m_device, chosenGraphicsQueueFamilyIndex, 0, &m_graphicsQueue);
		m_graphicsQueueFamilyIndex = chosenGraphicsQueueFamilyIndex;
		vkGetDeviceQueue(m_device, chosenTransferQueueFamilyIndex, 0, &m_asyncTransferQueue);
		m_asyncTransferQueueFamilyIndex = chosenTransferQueueFamilyIndex;

		vkGetPhysicalDeviceProperties(m_physicalDevice, &m_properties);

		m_frameCompletionFences.resize(frameInFlightCount);
		VkFenceCreateInfo fenceCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.flags = VK_FENCE_CREATE_SIGNALED_BIT
		};
		for(uint32_t i = 0; i < frameInFlightCount; ++i) {
			verifyResult(vkCreateFence(m_device, &fenceCreateInfo, nullptr, &m_frameCompletionFences[i]));
		}
	}
} // namespace vanadium::graphics