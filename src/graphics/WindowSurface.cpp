#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#include <GLFW/glfw3.h>

#include <graphics/WindowSurface.hpp>
#include <graphics/helper/EnumerationHelper.hpp>
#include <graphics/helper/ErrorHelper.hpp>
#include <volk.h>

using namespace vanadium::windowing;

namespace vanadium::graphics {
	void sizeChangeCallback(uint32_t width, uint32_t height, void* userData) {
		WindowSurface* surface = reinterpret_cast<WindowSurface*>(userData);
		surface->setSuggestedSize(width, height);
	}

	WindowSurface::WindowSurface(WindowInterface& interface) : m_interface(interface) {
		interface.windowSize(m_suggestedWidth, m_suggestedHeight);
		interface.addSizeListener({ .eventCallback = sizeChangeCallback,
									.listenerDestroyCallback = emptyListenerDestroyCallback,
									.userData = this });
	}

	void WindowSurface::create(VkInstance instance, size_t frameInFlightCount) {
		verifyResult(glfwCreateWindowSurface(instance, m_interface.internalHandle(), nullptr, &m_surface));
		m_frameInFlightCount = frameInFlightCount;
	}

	bool WindowSurface::supportsPresent(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex) {
		VkBool32 presentSupport;
		verifyResult(
			vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueFamilyIndex, m_surface, &presentSupport));
		return presentSupport;
	}

	void WindowSurface::createSwapchain(VkPhysicalDevice physicalDevice, VkDevice device,
										VkImageUsageFlags usageFlags) {
		if (m_acquireSemaphores.empty()) {
			m_acquireSemaphores.resize(m_frameInFlightCount);
			m_presentSemaphores.resize(m_frameInFlightCount);

			VkSemaphoreCreateInfo info = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

			for (size_t i = 0; i < m_acquireSemaphores.size(); ++i) {
				verifyResult(vkCreateSemaphore(device, &info, nullptr, &m_acquireSemaphores[i]));
			}
			for (size_t i = 0; i < m_presentSemaphores.size(); ++i) {
				verifyResult(vkCreateSemaphore(device, &info, nullptr, &m_presentSemaphores[i]));
			}
		}
		std::vector<VkSurfaceFormatKHR> surfaceFormats = enumerate<VkPhysicalDevice, VkSurfaceFormatKHR, VkSurfaceKHR>(
			physicalDevice, m_surface, vkGetPhysicalDeviceSurfaceFormatsKHR);

		if (std::find_if(surfaceFormats.begin(), surfaceFormats.end(), [](const auto& format) {
				return format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR &&
					   format.format == VK_FORMAT_B8G8R8A8_SRGB;
			}) == surfaceFormats.end()) {
			std::exit(-6);
		}

		VkSurfaceCapabilitiesKHR surfaceCapabilities;
		verifyResult(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, m_surface, &surfaceCapabilities));

		m_actualWidth = std::clamp(m_suggestedWidth, surfaceCapabilities.minImageExtent.width,
								   surfaceCapabilities.maxImageExtent.height);
		m_actualHeight = std::clamp(m_suggestedHeight, surfaceCapabilities.minImageExtent.height,
									surfaceCapabilities.maxImageExtent.height);

		if (m_actualWidth == 0 || m_actualHeight == 0) {
			m_canRender = false;
			return;
		}
		uint32_t imageCount = std::max(3U, surfaceCapabilities.minImageCount);
		if (surfaceCapabilities.maxImageCount) {
			imageCount = std::min(imageCount, surfaceCapabilities.maxImageCount);
		}
		VkSwapchainKHR oldSwapchain = m_swapchain;

		VkSwapchainCreateInfoKHR swapchainCreateInfo = { .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
														 .surface = m_surface,
														 .minImageCount = imageCount,
														 .imageFormat = VK_FORMAT_B8G8R8A8_SRGB,
														 .imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
														 .imageExtent = { .width = m_actualWidth,
																		  .height = m_actualHeight },
														 .imageArrayLayers = 1,
														 .imageUsage = usageFlags,
														 .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
														 .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
														 .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
														 .presentMode = VK_PRESENT_MODE_FIFO_KHR,
														 .clipped = VK_TRUE,
														 .oldSwapchain = m_swapchain };
		verifyResult(vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &m_swapchain));

		vkDestroySwapchainKHR(device, oldSwapchain, nullptr);
	}

	std::vector<VkImage> WindowSurface::swapchainImages(VkDevice device) {
		return enumerate<VkDevice, VkImage, VkSwapchainKHR>(device, m_swapchain, vkGetSwapchainImagesKHR);
	}

	void WindowSurface::setSuggestedSize(uint32_t width, uint32_t height) {
		m_suggestedWidth = width;
		m_suggestedHeight = height;
		m_swapchainDirtyFlag = true;
	}

	bool WindowSurface::refreshSwapchain(VkPhysicalDevice physicalDevice, VkDevice device,
										 VkImageUsageFlags usageFlags) {
		if (m_swapchainDirtyFlag) {
			createSwapchain(physicalDevice, device, usageFlags);
			m_swapchainDirtyFlag = false;
			return true;
		}
		return false;
	}

	uint32_t WindowSurface::tryAcquire(VkDevice device, uint32_t frameIndex) {
		if (m_lastFailedPresentIndex.has_value()) {
			uint32_t result = m_lastFailedPresentIndex.value();
			m_lastFailedPresentIndex = std::nullopt;
			return result;
		}
		uint32_t imageIndex;
		VkResult result = vkAcquireNextImageKHR(device, m_swapchain, UINT64_MAX, m_acquireSemaphores[frameIndex],
												VK_NULL_HANDLE, &imageIndex);
		switch (result) {
			case VK_ERROR_OUT_OF_DATE_KHR:
				m_canRender = false;
				m_swapchainDirtyFlag = true;
				break;
			case VK_SUBOPTIMAL_KHR:
				m_canRender = true;
				m_swapchainDirtyFlag = true;
				break;
			default:
				verifyResult(result);
				break;
		}
		return imageIndex;
	}

	void WindowSurface::tryPresent(VkQueue queue, uint32_t imageIndex, uint32_t frameIndex) {
		VkResult result;
		VkPresentInfoKHR presentInfo = { .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
										 .waitSemaphoreCount = 1,
										 .pWaitSemaphores = &m_presentSemaphores[frameIndex],
										 .swapchainCount = 1,
										 .pSwapchains = &m_swapchain,
										 .pImageIndices = &imageIndex,
										 .pResults = &result };
		vkQueuePresentKHR(queue, &presentInfo);
		switch (result) {
			case VK_ERROR_OUT_OF_DATE_KHR:
				m_canRender = false;
				m_swapchainDirtyFlag = true;
				m_lastFailedPresentIndex = imageIndex;
				break;
			case VK_SUBOPTIMAL_KHR:
				m_canRender = true;
				m_swapchainDirtyFlag = true;
				break;
			default:
				verifyResult(result);
				break;
		}
	}

	void WindowSurface::destroy(VkDevice device, VkInstance instance) {
		for(auto& semaphore : m_acquireSemaphores) {
			vkDestroySemaphore(device, semaphore, nullptr);
		}
		for(auto& semaphore : m_presentSemaphores) {
			vkDestroySemaphore(device, semaphore, nullptr);
		}
		vkDestroySwapchainKHR(device, m_swapchain, nullptr);
		vkDestroySurfaceKHR(instance, m_surface, nullptr);
	}
} // namespace vanadium::graphics
