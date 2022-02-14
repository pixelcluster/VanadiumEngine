#pragma once

#include <string_view>
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <modules/window/VWindowModule.hpp>

constexpr uint32_t frameInFlightCount = 3U;

struct VGPUCapabilities {
	bool memoryBudget;
	bool memoryPriority;
};

enum class SwapchainState {
	OK, Suboptimal, Invalid
};

struct AcquireResult {
	SwapchainState swapchainState;
	uint32_t imageIndex = -1U;
	uint32_t frameIndex;
	VkSemaphore imageSemaphore;
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

	VkQueue graphicsQueue() { return m_graphicsQueue; }
	uint32_t graphicsQueueFamilyIndex() { return m_graphicsQueueFamilyIndex; }

	VkFence frameCompletionFence() { return m_frameCompletionFences[m_frameIndex]; }

	AcquireResult acquireImage();
	SwapchainState presentImage(uint32_t imageIndex, VkSemaphore waitSemaphore);
	bool recreateSwapchain(VWindowModule* windowModule, VkImageUsageFlags imageUsageFlags);

	uint32_t frameIndex() const { return m_frameIndex; }

	const std::vector<VkImage>& swapchainImages() { return m_swapchainImages; }

	VkFormat swapchainImageFormat() const { return VK_FORMAT_B8G8R8A8_SRGB; }

	VkCommandPool perFrameCommandPool(uint32_t frameIndex) { return m_frameCommandPools[frameIndex]; }
	VkCommandBuffer allocateAdditionalCommandBuffer(uint32_t frameIndex);

	void submitAdditionalCommandBuffer(VkCommandBuffer bufferToSubmit);

	const std::vector<VkCommandBuffer>& additionalCommandBuffersToSubmit();

  private:
	VGPUCapabilities m_capabilities = {};

	VkInstance m_instance = VK_NULL_HANDLE;

	VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;

	VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
	VkDevice m_device = VK_NULL_HANDLE;

	VkQueue m_graphicsQueue = VK_NULL_HANDLE;
	uint32_t m_graphicsQueueFamilyIndex = 0;

	VkSurfaceKHR m_surface = VK_NULL_HANDLE;
	VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;

	VkSemaphore m_swapchainImageSemaphores[frameInFlightCount];
	uint32_t m_frameIndex;

	VkCommandPool m_frameCommandPools[frameInFlightCount];
	std::vector<VkCommandBuffer> m_freeCommandBuffers[frameInFlightCount];
	std::vector<VkCommandBuffer> m_additionalCommandBufferFreeList[frameInFlightCount];
	std::vector<VkCommandBuffer> m_buffersToSubmit;
	VkFence m_frameCompletionFences[frameInFlightCount];

	std::vector<VkImage> m_swapchainImages;
};