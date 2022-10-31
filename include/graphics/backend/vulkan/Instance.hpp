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

#include <graphics/frontend/Instance.hpp>
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

namespace vanadium::graphics::rhi::vulkan {
	class Instance : public rhi::Instance {
	  public:
		static Instance* create(const std::string_view& appName, uint32_t appVersion);

		virtual Device* createDevice(const PhysicalDevice& device) override;
		virtual Surface* createSurface(windowing::WindowInterface* interface) override;

	  private:
		Instance(const std::string_view& appName, uint32_t appVersion);

		VkInstance m_instance;

		VkDebugUtilsMessengerEXT m_debugMessenger;
	};
} // namespace vanadium::graphics::rhi::vulkan
