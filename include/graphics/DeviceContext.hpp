#pragma once
#define VK_NO_PROTOTYPES
#include <graphics/WindowSurface.hpp>
#include <string_view>
#include <vulkan/vulkan.h>

namespace vanadium::graphics {

	constexpr uint32_t frameInFlightCount = 3U;

	struct DeviceCapabilities {
		bool memoryBudget;
		bool memoryPriority;
	};

	class DeviceContext {
	  public:
		DeviceContext(const std::string_view& appName, uint32_t appVersion, WindowSurface& windowSurface);
		DeviceContext(const DeviceContext&) = delete;
		DeviceContext(DeviceContext&&) = delete;
		~DeviceContext();

		VkInstance instance() { return m_instance; }
		VkPhysicalDevice physicalDevice() { return m_physicalDevice; }
		VkDevice device() { return m_device; }

		uint32_t graphicsQueueFamilyIndex() const { return m_graphicsQueueFamilyIndex; }
		VkQueue graphicsQueue() { return m_graphicsQueue; }

		DeviceCapabilities deviceCapabilities() const { return m_capabilities; }

	  private:
		VkInstance m_instance;
		VkPhysicalDevice m_physicalDevice;
		VkDevice m_device;

		uint32_t m_graphicsQueueFamilyIndex;
		VkQueue m_graphicsQueue;

		VkDebugUtilsMessengerEXT m_debugMessenger;

		DeviceCapabilities m_capabilities;
	};
} // namespace vanadium::graphics