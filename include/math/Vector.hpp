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

#include <Log.hpp>
#include <concepts>
#include <cstdint>
#include <util/CompilerSpecific.hpp>

namespace vanadium {
	template <uint32_t n, std::floating_point T> struct Vector;

	template <std::floating_point T> struct Vector<2, T> {
		constexpr Vector() {}
		constexpr Vector(T n) : x(n), y(n) {}
		constexpr Vector(T _x, T _y) : x(_x), y(_y) {}

		constexpr bool operator==(const Vector<2, T>& other) const { return x == other.x && y == other.y; }
		constexpr bool operator!=(const Vector<2, T>& other) const { return !(*this == other); }
		constexpr Vector<2, T> operator+(const Vector<2, T>& other) const { return Vector<2, T>(x + other.x, y + other.y); }
		constexpr Vector<2, T> operator-(const Vector<2, T>& other) const { return Vector<2, T>(x - other.x, y - other.y); }
		constexpr Vector<2, T> operator*(const Vector<2, T>& other) const { return Vector<2, T>(x * other.x, y * other.y); }
		constexpr Vector<2, T> operator/(const Vector<2, T>& other) const { return Vector<2, T>(x / other.x, y / other.y); }
		constexpr float operator[](uint32_t index) const {
			float result;
			switch (index) {
				case 0:
					result = x;
					break;
				case 1:
					result = y;
					break;
				default:
					result = 0.0f;
					break;
			}
			return result;
		}
		float& operator[](uint32_t index) {
			assertFatal(index < 2, SubsystemID::Unknown, "OOB vector access!");
			switch (index) {
				case 0:
					return x;
				case 1:
					return y;
				default:
					UNREACHABLE
			}
		}
		constexpr float dot(const Vector<2, T>& other) { return x * other.x + y * other.y; }

		union {
			T x;
			T r;
			T s;
		};

		union {
			T y;
			T g;
			T t;
		};
	};

	template <std::floating_point T> struct Vector<3, T> {
		constexpr Vector() {}
		constexpr Vector(T n) : x(n), y(n), z(n) {}
		constexpr Vector(T _x, T _y, T _z) : x(_x), y(_y), z(_z) {}
		constexpr Vector(Vector<2, T> xy, T _z) : x(xy.x), y(xy.y), z(_z) {}
		constexpr Vector(T _x, Vector<2, T> yz) : x(x), y(yz.x), z(yz.y) {}

		constexpr bool operator==(const Vector<3, T>& other) const {
			return x == other.x && y == other.y && z == other.z;
		}
		constexpr bool operator!=(const Vector<3, T>& other) const { return !(*this == other); }
		constexpr Vector<3, T> operator+(const Vector<3, T>& other) const {
			return Vector<3, T>(x + other.x, y + other.y, z + other.z);
		}
		constexpr Vector<3, T> operator-(const Vector<3, T>& other) const {
			return Vector<3, T>(x - other.x, y - other.y, z - other.z);
		}
		constexpr Vector<3, T> operator*(const Vector<3, T>& other) const {
			return Vector<3, T>(x * other.x, y * other.y, z * other.z);
		}
		constexpr Vector<3, T> operator/(const Vector<3, T>& other) const {
			return Vector<3, T>(x / other.x, y / other.y, z / other.z);
		}
		constexpr float operator[](uint32_t index) const {
			float result;
			switch (index) {
				case 0:
					result = x;
					break;
				case 1:
					result = y;
					break;
				case 2:
					result = z;
					break;
				default:
					result = 0.0f;
					break;
			}
			return result;
		}
		float& operator[](uint32_t index) {
			assertFatal(index < 3, SubsystemID::Unknown, "OOB vector access!");
			switch (index) {
				case 0:
					return x;
				case 1:
					return y;
				case 2:
					return z;
				default:
					UNREACHABLE
			}
		}
		constexpr float dot(const Vector<3, T>& other) { return x * other.x + y * other.y + z * other.z; }

		union {
			T x;
			T r;
			T s;
		};

		union {
			T y;
			T g;
			T t;
		};

		union {
			T z;
			T b;
			T p;
		};
	};

	template <std::floating_point T> struct Vector<4, T> {
		constexpr Vector() {}
		constexpr Vector(T n) : x(n), y(n), z(n), w(n) {}
		constexpr Vector(T _x, T _y, T _z, T _w) : x(_x), y(_y), z(_z), w(_w) {}
		constexpr Vector(Vector<2, T> xy, Vector<2, T> zw) : x(xy.x), y(xy.y), z(zw.x), w(zw.y) {}
		constexpr Vector(T _x, Vector<3, T> yzw) : x(x), y(yzw.x), z(yzw.y), w(yzw.z) {}
		constexpr Vector(Vector<3, T> xyz, T _w) : x(xyz.x), y(xyz.y), z(xyz.z), w(_w) {}

		constexpr bool operator==(const Vector<4, T>& other) const {
			return x == other.x && y == other.y && z == other.z && w == other.w;
		}
		constexpr bool operator!=(const Vector<4, T>& other) const { return !(*this == other); }
		constexpr Vector<4, T> operator+(const Vector<4, T>& other) const {
			return Vector<4, T>(x + other.x, y + other.y, z + other.z, w + other.w);
		}
		constexpr Vector<4, T> operator-(const Vector<4, T>& other) const {
			return Vector<4, T>(x - other.x, y - other.y, z - other.z, w - other.w);
		}
		constexpr Vector<4, T> operator*(const Vector<4, T>& other) const {
			return Vector<4, T>(x * other.x, y * other.y, z * other.z, w * other.w);
		}
		constexpr Vector<4, T> operator/(const Vector<4, T>& other) const {
			return Vector<4, T>(x / other.x, y / other.y, z / other.z, w / other.w);
		}
		constexpr float operator[](uint32_t index) const {
			float result;
			switch (index) {
				case 0:
					result = x;
					break;
				case 1:
					result = y;
					break;
				case 2:
					result = z;
					break;
				case 3:
					result = w;
					break;
				default:
					result = 0.0f;
					break;
			}
			return result;
		}
		float& operator[](uint32_t index) {
			assertFatal(index < 4, SubsystemID::Unknown, "OOB vector access!");
			switch (index) {
				case 0:
					return x;
				case 1:
					return y;
				case 2:
					return z;
				case 3:
					return w;
				default:
					UNREACHABLE
			}
		}
		constexpr float dot(const Vector<4, T>& other) { return x * other.x + y * other.y + z * other.z + w * other.w; }

		union {
			T x;
			T r;
			T s;
		};

		union {
			T y;
			T g;
			T t;
		};

		union {
			T z;
			T b;
			T p;
		};

		union {
			T w;
			T a;
			T q;
		};
	};

	using Vector2 = Vector<2, float>;
	using Vector3 = Vector<3, float>;
	using Vector4 = Vector<4, float>;
} // namespace vanadium
