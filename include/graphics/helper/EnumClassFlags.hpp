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

#define DEFINE_FLAGS(enum_type) \
inline enum_type operator|(const enum_type& one, const enum_type& other) { \
	return static_cast<enum_type>(static_cast<uint64_t>(one) | static_cast<uint64_t>(other)); \
} \
\
inline enum_type operator&(const enum_type& one, const enum_type& other) { \
	return static_cast<enum_type>(static_cast<uint64_t>(one) & static_cast<uint64_t>(other)); \
} \
\
inline enum_type operator^(const enum_type& one, const enum_type& other) { \
	return static_cast<enum_type>(static_cast<uint64_t>(one) ^ static_cast<uint64_t>(other)); \
}