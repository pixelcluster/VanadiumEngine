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