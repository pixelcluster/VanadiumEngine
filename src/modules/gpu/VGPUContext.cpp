#include <VDebug.hpp>
#include <volk.h>

#include <modules/gpu/VGPUContext.hpp>

#include <modules/gpu/helper/EnumerationHelper.hpp>
#include <modules/gpu/helper/ErrorHelper.hpp>

#include <algorithm>
#include <optional>
#include <vector>

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

void VGPUContext::create(const std::string_view& appName, uint32_t appVersion, VWindowModule* windowModule) {
	VkApplicationInfo appInfo = { .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
								  .pApplicationName = appName.data(),
								  .applicationVersion = appVersion,
								  .pEngineName = "Vanadium",
								  .engineVersion = 1,
								  .apiVersion = VK_API_VERSION_1_0 };

	std::vector<const char*> instanceExtensionNames = { platformSurfaceExtensionName(), "VK_KHR_surface" };

	VkInstanceCreateInfo instanceCreateInfo = { .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
												.pApplicationInfo = &appInfo };

	VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
		.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
						   VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
						   VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT,
		.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
					   VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
	};

	if constexpr (vanadiumGPUDebug) {
		instanceCreateInfo.pNext = &debugUtilsMessengerCreateInfo;
		instanceExtensionNames.push_back("VK_EXT_debug_utils");
	}

	instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensionNames.size()),
	instanceCreateInfo.ppEnabledExtensionNames = instanceExtensionNames.data();

	verifyResult(volkInitialize());
	verifyResult(vkCreateInstance(&instanceCreateInfo, nullptr, &m_instance));

	volkLoadInstanceOnly(m_instance);

	if constexpr (vanadiumGPUDebug) {
		vkCreateDebugUtilsMessengerEXT(m_instance, &debugUtilsMessengerCreateInfo, nullptr, &m_debugMessenger);
	}

	m_surface = windowModule->createWindowSurface(m_instance);

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
			vkGetPhysicalDeviceSurfaceSupportKHR(device, queueFamilyIndex, m_surface, &surfaceSupport);
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
											.enabledExtensionCount = static_cast<uint32_t>(deviceExtensionNames.size()),
											.ppEnabledExtensionNames = deviceExtensionNames.data() };
	verifyResult(vkCreateDevice(chosenDevice.value(), &deviceCreateInfo, nullptr, &m_device));

	volkLoadDevice(m_device);

	m_physicalDevice = chosenDevice.value();
	std::vector<VkSurfaceFormatKHR> surfaceFormats = enumerate<VkPhysicalDevice, VkSurfaceFormatKHR, VkSurfaceKHR>(
		m_physicalDevice, m_surface, vkGetPhysicalDeviceSurfaceFormatsKHR);

	if (std::find_if(surfaceFormats.begin(), surfaceFormats.end(), [](const auto& format) {
			return format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR && format.format == VK_FORMAT_B8G8R8A8_SRGB;
		}) == surfaceFormats.end()) {
		std::exit(-6);
	}

	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface, &surfaceCapabilities);

	uint32_t minImageCount = std::max(3U, surfaceCapabilities.minImageCount);

	if (surfaceCapabilities.maxImageCount) {
		minImageCount = std::min(minImageCount, surfaceCapabilities.maxImageCount);
	}

	VkSwapchainCreateInfoKHR swapchainCreateInfo = { .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
													 .surface = m_surface,
													 .minImageCount = minImageCount,
													 .imageFormat = VK_FORMAT_R8G8B8A8_SRGB,
													 .imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
													 .imageExtent = { .width = windowModule->width(),
																	  .height = windowModule->height() },
													 .imageArrayLayers = 1,
													 .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
													 .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
													 .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
													 .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
													 .clipped = VK_TRUE };
	verifyResult(vkCreateSwapchainKHR(m_device, &swapchainCreateInfo, nullptr, &m_swapchain));

	vkGetDeviceQueue(m_device, chosenQueueFamilyIndex, 0, &m_graphicsQueue);

	VkCommandPoolCreateInfo poolCreateInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
											   .queueFamilyIndex = chosenQueueFamilyIndex };

	VkFenceCreateInfo fenceCreateInfo = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
										  .flags = VK_FENCE_CREATE_SIGNALED_BIT };

	VkSemaphoreCreateInfo semaphoreCreateInfo = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

	for (size_t i = 0; i < frameInFlightCount; ++i) {
		verifyResult(vkCreateCommandPool(m_device, &poolCreateInfo, nullptr, &m_frameCommandPools[i]));

		VkCommandBufferAllocateInfo allocateInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
													 .commandPool = m_frameCommandPools[i],
													 .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY };
		verifyResult(vkAllocateCommandBuffers(m_device, &allocateInfo, &m_frameCommandBuffers[i]));

		verifyResult(vkCreateFence(m_device, &fenceCreateInfo, nullptr, &m_frameCompletionFences[i]));
		verifyResult(vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &m_swapchainImageSemaphores[i]));
	}

	m_frameIndex = 0;
}

void VGPUContext::destroy() {
	vkDeviceWaitIdle(m_device);

	vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
	vkDestroyDevice(m_device, nullptr);

	if constexpr (vanadiumGPUDebug) {
		vkDestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
	}
	vkDestroyInstance(m_instance, nullptr);
}

AcquireResult VGPUContext::acquireImage() {
	verifyResult(vkWaitForFences(m_device, 1, &m_frameCompletionFences[m_frameIndex], VK_TRUE, UINT64_MAX));

	verifyResult(vkResetCommandPool(m_device, m_frameCommandPools[m_frameIndex], 0));

	AcquireResult acquireResult;
	VkResult result = vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, m_swapchainImageSemaphores[m_frameIndex],
											VK_NULL_HANDLE, &acquireResult.imageIndex);

	switch (result) {
		case VK_SUBOPTIMAL_KHR:
			acquireResult.swapchainState = SwapchainState::Suboptimal;
			break;
		case VK_ERROR_OUT_OF_DATE_KHR:
			acquireResult.swapchainState = SwapchainState::Invalid;
			break;
		default:
			verifyResult(result);
			break;
	}
	acquireResult.frameIndex = m_frameIndex;
	acquireResult.imageSemaphore = m_swapchainImageSemaphores[m_frameIndex];
	return acquireResult;
}

SwapchainState VGPUContext::presentImage(uint32_t imageIndex, VkSemaphore semaphore) {
	VkResult presentResult;
	VkPresentInfoKHR presentInfo = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &semaphore,
		.swapchainCount = 1,
		.pSwapchains = &m_swapchain,
		.pImageIndices = &imageIndex,
		.pResults = &presentResult
	};

	vkQueuePresentKHR(m_graphicsQueue, &presentInfo);

	switch (presentResult) {
		case VK_SUBOPTIMAL_KHR:
			return SwapchainState::Suboptimal;
			break;
		case VK_ERROR_OUT_OF_DATE_KHR:
			return SwapchainState::Invalid;
			break;
		default:
			return SwapchainState::OK;
			break;
	}

	++m_frameIndex %= 3;
}
