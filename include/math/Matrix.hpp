#pragma once
#include <math/Vector.hpp>

namespace vanadium {

	template <uint32_t M, uint32_t N, std::floating_point T> struct Matrix {
		constexpr Matrix() {
			for (uint32_t i = 0; i < N; ++i) {
				for (uint32_t j = 0; j < M; ++j) {
					elements[i * M + j] = 0.0f;
				}
			}
		}

		template <typename... Ts>
		requires(sizeof...(Ts) == M * N && (std::is_same_v<T, Ts> && ...)) constexpr Matrix(Ts... members) {
			uint32_t i = 0;
			(assign(i++, members), ...);
		}
		template <typename... Ts>
		requires(sizeof...(Ts) == N && (std::is_same_v<Vector<M, T>, Ts> && ...)) constexpr Matrix(Ts... vectors) {
			for (uint32_t i = 0; i < M; ++i) {
				uint32_t j = 0;
				(assign(j++ * M + i, vectors[i]), ...);
			}
		}
		constexpr Matrix(T value) {
			for (uint32_t i = 0; i < N; ++i) {
				for (uint32_t j = 0; j < M; ++j) {
					if (i == j)
						elements[i * M + j] = value;
					else
						elements[i * M + j] = static_cast<T>(0.0);
				}
			}
		}
		constexpr Matrix<M, N, T> operator*(const Matrix<N, M, T>& other) {
			Matrix<M, N, T> result;
			for (uint32_t i = 0; i < N; ++i) {
				for (uint32_t j = 0; j < M; ++j) {
					float value = 0.0f;
					for (uint32_t k = 0; k < M; ++k) {
						value += elements[i * M + k] * other.elements[k * N + j];
					}
					result.elements[i * M + j] = value;
				}
			}
			return result;
		}
		constexpr Matrix<M, N, T>& operator*=(const Matrix<N, M, T>& other) {
			for (uint32_t i = 0; i < N; ++i) {
				for (uint32_t j = 0; j < M; ++j) {
					float value = 0.0f;
					for (uint32_t k = 0; k < M; ++k) {
						value += elements[i * M + k] * other.elements[k * N + j];
					}
					elements[i * M + j] = value;
				}
			}
			return *this;
		}
		constexpr Vector<N, T> operator*(const Vector<M, T>& other) {
			Vector<N, T> result;
			for (uint32_t i = 0; i < N; ++i) {
				float value = 0.0f;
				for (uint32_t j = 0; j < M; ++j) {
					value += elements[i * M + j] * other[j];
				}
				result[i] = value;
			}
			return result;
		}
		constexpr T& at(uint32_t row, uint32_t column) { return elements[row * M + column]; }

	  private:
		T elements[M * N];

		// helper for fold expressions
		void assign(uint32_t index, T value) { elements[index] = value; }
	};

	using Matrix2 = Matrix<2, 2, float>;
	using Matrix3 = Matrix<3, 3, float>;
	using Matrix4 = Matrix<4, 4, float>;

	using Matrix2x2 = Matrix<2, 2, float>;
	using Matrix3x3 = Matrix<3, 3, float>;
	using Matrix4x4 = Matrix<4, 4, float>;

	using Matrix3x4 = Matrix<3, 4, float>;
	using Matrix2x4 = Matrix<2, 4, float>;

	using Matrix2x3 = Matrix<2, 3, float>;
	using Matrix4x3 = Matrix<4, 3, float>;

	using Matrix3x2 = Matrix<3, 2, float>;
	using Matrix4x2 = Matrix<2, 4, float>;

} // namespace vanadium
