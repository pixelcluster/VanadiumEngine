#pragma once

#include <concepts>
#include <cstdint>

namespace vanadium {
	template <uint32_t n, std::floating_point T> struct Vector;

	template <std::floating_point T> struct Vector<2, T> {
		constexpr Vector() {}
		constexpr Vector(T n) : x(n), y(n) {}
		constexpr Vector(T _x, T _y) : x(_x), y(_y) {}

		bool operator==(const Vector<2, T>& other) const { return x == other.x && y == other.y; }
		bool operator!=(const Vector<2, T>& other) const { return !(*this == other); }

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

		bool operator==(const Vector<3, T>& other) const { return x == other.x && y == other.y && z == other.z; }
		bool operator!=(const Vector<3, T>& other) const { return !(*this == other); }

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

		bool operator==(const Vector<4, T>& other) const {
			return x == other.x && y == other.y && z == other.z && w == other.w;
		}
		bool operator!=(const Vector<4, T>& other) const { return !(*this == other); }

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