#include <VDebug.hpp>
#include <volk.h>

#include <modules/gpu/VGPUContext.hpp>

#include <modules/gpu/helper/EnumerationHelper.hpp>
#include <modules/gpu/helper/ErrorHelper.hpp>

#include <algorithm>
#include <iostream>
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

VKAPI_ATTR VkBool32 VKAPI_CALL debugLog(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
										VkDebugUtilsMessageTypeFlagsEXT messageTypes,
										const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
	std::cout << pCallbackData->pMessage << "\n";
	return VK_FALSE;
}

void VGPUContext::create(const std::string_view& appName, uint32_t appVersion, VWindowModule* windowModule) {
	verifyResult(volkInitialize());

	VkApplicationInfo appInfo = { .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
								  .pApplicationName = appName.data(),
								  .applicationVersion = appVersion,
								  .pEngineName = "Vanadium",
								  .engineVersion = 1,
								  .apiVersion = VK_API_VERSION_1_0 };

	std::vector<const char*> instanceExtensionNames = { platformSurfaceExtensionName(), "VK_KHR_surface" };

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
		.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
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

	m_frameIndex = 0;
}

void VGPUContext::destroy() {
	for (auto& pool : m_frameCommandPools) {
		vkDestroyCommandPool(m_device, pool, nullptr);
	}

	for (auto& semaphore : m_swapchainImageSemaphores) {
		vkDestroySemaphore(m_device, semaphore, nullptr);
	}

	for (auto& fence : m_frameCompletionFences) {
		vkDestroyFence(m_device, fence, nullptr);
	}

	vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
	vkDestroyDevice(m_device, nullptr);

	vkDestroySurfaceKHR(m_instance, m_surface, nullptr);

	if constexpr (vanadiumGPUDebug) {
		vkDestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
	}
	vkDestroyInstance(m_instance, nullptr);
}

AcquireResult VGPUContext::acquireImage() {
	vkResetCommandPool(m_device, m_frameCommandPools[m_frameIndex], 0);
	m_buffersToSubmit.clear();
	for (auto& buffer : m_additionalCommandBufferFreeList[m_frameIndex]) {
		m_freeCommandBuffers[m_frameIndex].push_back(buffer);
	}
	m_additionalCommandBufferFreeList[m_frameIndex].clear();

	verifyResult(vkWaitForFences(m_device, 1, &m_frameCompletionFences[m_frameIndex], VK_TRUE, UINT64_MAX));
	verifyResult(vkResetFences(m_device, 1, &m_frameCompletionFences[m_frameIndex]));

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
			acquireResult.swapchainState = SwapchainState::OK;
			verifyResult(result);
			break;
	}
	acquireResult.frameIndex = m_frameIndex;
	acquireResult.imageSemaphore = m_swapchainImageSemaphores[m_frameIndex];
	return acquireResult;
}

SwapchainState VGPUContext::presentImage(uint32_t imageIndex, VkSemaphore semaphore) {
	VkResult presentResult;
	VkPresentInfoKHR presentInfo = { .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
									 .waitSemaphoreCount = 1,
									 .pWaitSemaphores = &semaphore,
									 .swapchainCount = 1,
									 .pSwapchains = &m_swapchain,
									 .pImageIndices = &imageIndex,
									 .pResults = &presentResult };

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

bool VGPUContext::recreateSwapchain(VWindowModule* windowModule, VkImageUsageFlags imageUsageFlags) {
	verifyResult(vkDeviceWaitIdle(m_device));

	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface, &surfaceCapabilities);

	if (surfaceCapabilities.minImageExtent.width > windowModule->width() ||
		surfaceCapabilities.minImageExtent.height > windowModule->height() ||
		surfaceCapabilities.maxImageExtent.width < windowModule->width() ||
		surfaceCapabilities.maxImageExtent.height < windowModule->height() ||
		surfaceCapabilities.maxImageExtent.width == 0 || surfaceCapabilities.maxImageExtent.height == 0)
		return false;

	uint32_t minImageCount = std::max(3U, surfaceCapabilities.minImageCount);

	if (surfaceCapabilities.maxImageCount) {
		minImageCount = std::min(minImageCount, surfaceCapabilities.maxImageCount);
	}

	VkSwapchainKHR oldSwapchain = m_swapchain;

	VkSwapchainCreateInfoKHR swapchainCreateInfo = { .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
													 .surface = m_surface,
													 .minImageCount = minImageCount,
													 .imageFormat = VK_FORMAT_B8G8R8A8_SRGB,
													 .imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
													 .imageExtent = { .width = windowModule->width(),
																	  .height = windowModule->height() },
													 .imageArrayLayers = 1,
													 .imageUsage = imageUsageFlags,
													 .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
													 .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
													 .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
													 .presentMode = VK_PRESENT_MODE_FIFO_KHR,
													 .clipped = VK_TRUE,
													 .oldSwapchain = m_swapchain };
	verifyResult(vkCreateSwapchainKHR(m_device, &swapchainCreateInfo, nullptr, &m_swapchain));

	vkDestroySwapchainKHR(m_device, oldSwapchain, nullptr);

	uint32_t swapchainImageCount;
	verifyResult(vkGetSwapchainImagesKHR(m_device, m_swapchain, &swapchainImageCount, nullptr));

	m_swapchainImages.resize(swapchainImageCount);
	verifyResult(vkGetSwapchainImagesKHR(m_device, m_swapchain, &swapchainImageCount, m_swapchainImages.data()));
	return true;
}

VkCommandBuffer VGPUContext::allocateAdditionalCommandBuffer(uint32_t frameIndex) {
	if (m_freeCommandBuffers[frameIndex].empty()) {
		VkCommandBufferAllocateInfo allocateInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = m_frameCommandPools[frameIndex],
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1,
		};
		VkCommandBuffer newBuffer;
		verifyResult(vkAllocateCommandBuffers(m_device, &allocateInfo, &newBuffer));
		return newBuffer;
	} else {
		VkCommandBuffer buffer = m_freeCommandBuffers[frameIndex].back();
		m_freeCommandBuffers[frameIndex].pop_back();
		return buffer;
	}
}

void VGPUContext::submitAdditionalCommandBuffer(VkCommandBuffer bufferToSubmit) {
	m_buffersToSubmit.push_back(bufferToSubmit);
	m_additionalCommandBufferFreeList[m_frameIndex].push_back(bufferToSubmit);
}

const std::vector<VkCommandBuffer>& VGPUContext::additionalCommandBuffersToSubmit() {
	return m_buffersToSubmit;
}
