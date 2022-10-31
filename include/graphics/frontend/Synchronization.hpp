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

#include <cstddef>

namespace vanadium::graphics::rhi {

	class Semaphore {
	  public:
		virtual ~Semaphore() = 0;
	};

	class Fence {
	  public:
		virtual ~Fence() = 0;

		virtual bool isSignaled() const = 0;
		virtual void wait(size_t timeout) = 0;
	};
} // namespace vanadium::graphics::rhi
