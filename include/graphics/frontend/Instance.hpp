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

#include <cstdint>
#include <graphics/frontend/RefCounted.hpp>
#include <string>
#include <vector>

namespace windowing {
	class WindowInterface;
}

namespace vanadium::graphics::rhi {

	class Device;
	class Surface;

	struct PhysicalDevice {
		std::string name;
		uint64_t id;
	};

	class Instance : public RefCounted {
	  public:
		virtual ~Instance() = 0;

		const std::vector<PhysicalDevice>& physicalDevices() const { return m_devices; }

		virtual Device* createDevice(const PhysicalDevice& device) = 0;
		virtual Surface* createSurface(windowing::WindowInterface* interface) = 0;

	  protected:
		std::vector<PhysicalDevice> m_devices;
	};
} // namespace vanadium::graphics::rhi
