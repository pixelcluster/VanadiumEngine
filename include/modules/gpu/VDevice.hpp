#pragma once

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>

#include <modules/gpu/VQueueFamilyLayout.hpp>


namespace gpu {
	template <typename T>
	concept HasBaseStructMembers = requires(T t) {
		t.sType;
		t.pNext;
	};

	class VDevice {
	  public:
		VDevice(VkInstance instance, VkSurfaceKHR surface, const std::vector<VQueueFamilyLayout> queueFamilies,
			   const std::vector<std::string_view>& extensions, const VkPhysicalDeviceFeatures2& features);

		VkDevice device() { return m_device; }

		uint32_t deviceVersion() { return m_deviceProperties.apiVersion; }
		std::string_view deviceName() { return m_deviceProperties.deviceName; }

	  private:
		VkDevice m_device;

		VkPhysicalDeviceProperties m_deviceProperties;

		std::vector<std::string> m_enabledExtensions;
		std::vector<VQueueFamilyLayout> m_usedQueueFamilies;
	};
} // namespace gpu