/* VanadiumEngine, a Vulkan rendering toolkit
 * Copyright (C) 2022 Friedrich Vock
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
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

		VkInstance instance() { return m_instance; }
		VkPhysicalDevice physicalDevice() { return m_physicalDevice; }
		VkDevice device() { return m_device; }

		uint32_t graphicsQueueFamilyIndex() const { return m_graphicsQueueFamilyIndex; }
		VkQueue graphicsQueue() { return m_graphicsQueue; }

		uint32_t asyncTransferQueueFamilyIndex() const { return m_asyncTransferQueueFamilyIndex; }
		VkQueue asyncTransferQueue() { return m_asyncTransferQueue; }

		DeviceCapabilities deviceCapabilities() const { return m_capabilities; }
		const VkPhysicalDeviceProperties& properties() const { return m_properties; }

		const VkFence& frameCompletionFence(uint32_t frameIndex) { return m_frameCompletionFences[frameIndex]; }

		void destroy();

	  private:
		VkInstance m_instance;
		VkPhysicalDevice m_physicalDevice;
		VkPhysicalDeviceProperties m_properties;
		VkDevice m_device;

		uint32_t m_graphicsQueueFamilyIndex;
		VkQueue m_graphicsQueue;

		uint32_t m_asyncTransferQueueFamilyIndex;
		VkQueue m_asyncTransferQueue;

		VkDebugUtilsMessengerEXT m_debugMessenger;

		DeviceCapabilities m_capabilities;

		std::vector<VkFence> m_frameCompletionFences;
	};
} // namespace vanadium::graphics
