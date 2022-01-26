#pragma once

#include <string_view>
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <modules/window/VWindowModule.hpp>

struct VGPUCapabilities {
	bool memoryBudget;
	bool memoryPriority;
};

class VGPUContext {
  public:
	VGPUContext() {}
	VGPUContext(const VGPUContext& other) = delete;
	VGPUContext& operator=(const VGPUContext& other) = delete;
	VGPUContext(VGPUContext&& other) = delete;
	VGPUContext& operator=(VGPUContext&& other) = delete;

	void create(const std::string_view& appName, uint32_t appVersion, VWindowModule* windowModule);
	void destroy();

	const VGPUCapabilities& gpuCapabilities() const { return m_capabilities; };

	VkInstance instance() { return m_instance; }

	VkPhysicalDevice physicalDevice() { return m_physicalDevice; }
	VkDevice device() { return m_device; }

  private:
	VGPUCapabilities m_capabilities = {};

	VkInstance m_instance = VK_NULL_HANDLE;

	VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;

	VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
	VkDevice m_device = VK_NULL_HANDLE;

	VkSurfaceKHR m_surface = VK_NULL_HANDLE;
	VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
};