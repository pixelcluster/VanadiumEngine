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

#include <robin_hood.h>

namespace robin_hood
{
	template <std::integral T, std::integral U> constexpr size_t hashCombine(T t, U u) {
		if (t == 0)
			return u;
		if (u == 0)
			return t;
		return t ^ (0x517cc1b727220a95 + ((t << 2) ^ (u >> 3)));
	}

	template <std::integral T, std::integral... Ts> constexpr size_t hashCombine(T t, Ts... ts) {
		size_t u = hashCombine(ts...);
		return hashCombine(t, u);
	}
} // namespace robin_hood
