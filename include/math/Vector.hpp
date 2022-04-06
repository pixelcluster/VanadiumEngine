#pragma once

#include <concepts>
#include <cstdint>

namespace vanadium {
	template <uint32_t n, std::floating_point T> struct Vector;

	template <std::floating_point T> struct Vector<2, T> {
		Vector(T _x, T _y) : x(_x), y(_y) {}

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
		Vector(T _x, T _y, T _z) : x(_x), y(_y), z(_z) {}
		Vector(Vector<2, T> xy, T _z) : x(xy.x), y(xy.y), z(_z) {}
		Vector(T _x, Vector<2, T> yz) : x(x), y(yz.x), z(yz.y) {}

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
		Vector(T _x, T _y, T _z, T _w) : x(_x), y(_y), z(_z), w(_w) {}
		Vector(Vector<2, T> xy, Vector<2, T> zw) : x(xy.x), y(xy.y), z(zw.x), w(zw.y) {}
		Vector(T _x, Vector<3, T> yzw) : x(x), y(yzw.x), z(yzw.y), w(yzw.z) {}
		Vector(Vector<3, T> xyz, T _w) : x(xyz.x), y(xyz.y), z(xyz.z), w(_w) {}

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
