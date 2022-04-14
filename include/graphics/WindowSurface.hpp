#pragma once
#include <optional>
#include <windowing/WindowInterface.hpp>
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

namespace vanadium::graphics {
	class WindowSurface {
	  public:
		WindowSurface(windowing::WindowInterface& interface);

		void create(VkInstance instance, size_t frameInFlightCount);

		bool supportsPresent(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex);
		void createSwapchain(VkPhysicalDevice physicalDevice, VkDevice device, VkImageUsageFlags usageFlags);

		void destroy(VkDevice device, VkInstance instance);

		bool surfaceDirtyFlag() const { return m_surfaceDirtyFlag; }
		bool swapchainDirtyFlag() const { return m_swapchainDirtyFlag; }
		bool canRender() const { return m_canRender; }

		std::vector<VkImage> swapchainImages(VkDevice device);
		VkFormat swapchainImageFormat() { return VK_FORMAT_B8G8R8A8_SRGB; }

		uint32_t imageWidth() const { return m_actualWidth; }
		uint32_t imageHeight() const { return m_actualHeight; }

		void setSuggestedSize(uint32_t suggestedWidth, uint32_t suggestedHeight);
		void updateActualSize(VkPhysicalDevice physicalDevice);

		// returns whether the swapchain data was changed
		bool refreshSwapchain(VkPhysicalDevice physicalDevice, VkDevice device, VkImageUsageFlags usageFlags);

		uint32_t tryAcquire(VkDevice device, uint32_t frameIndex);
		void tryPresent(VkQueue queue, uint32_t imageIndex, uint32_t frameIndex);

		const VkSemaphore& acquireSemaphore(uint32_t frameIndex) const { return m_acquireSemaphores[frameIndex]; }
		const VkSemaphore& presentSemaphore(uint32_t frameIndex) const { return m_presentSemaphores[frameIndex]; }

	  private:
		windowing::WindowInterface& m_interface;

		bool m_surfaceDirtyFlag = true;
		bool m_swapchainDirtyFlag = true;
		bool m_canRender = false;

		uint32_t m_suggestedWidth = 0, m_suggestedHeight = 0;
		uint32_t m_actualWidth = 0, m_actualHeight = 0;

		size_t m_frameInFlightCount;
		std::vector<VkSemaphore> m_acquireSemaphores;
		std::vector<VkSemaphore> m_presentSemaphores;

		VkSurfaceKHR m_surface = VK_NULL_HANDLE;
		VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
	};

} // namespace vanadium::graphics
