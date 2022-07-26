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
#include <TestList.hpp>
#include <TestUtilCommon.hpp>
#include <math/Matrix.hpp>

using namespace vanadium;

void testMatrixConstructor() {
	Matrix2 mat = Matrix2(1.0f, 2.0f, 3.0f, 4.0f);
	testEqual(1.0f, mat.at(0, 0), "First element doesn't match!");
	testEqual(2.0f, mat.at(0, 1), "Second element doesn't match!");
	testEqual(3.0f, mat.at(1, 0), "Third element doesn't match!");
	testEqual(4.0f, mat.at(1, 1), "Fourth element doesn't match!");
}

void testMatrixMultiplication() {
	Matrix2 mat1 = Matrix2(2.0f, 1.0f, 3.0f, 5.0f);
	Matrix2 mat2 = Matrix2(6.0f, 2.0f, 7.0f, 3.0f);
	Matrix2 mat = mat1 * mat2;
	testEqual(19.0f, mat.at(0, 0), "First element doesn't match!");
	testEqual(7.0f, mat.at(0, 1), "Second element doesn't match!");
	testEqual(53.0f, mat.at(1, 0), "Third element doesn't match!");
	testEqual(21.0f, mat.at(1, 1), "Fourth element doesn't match!");
}

void testMatrixVectorMultiplication() {
    // clang-format off
	Matrix4 mat =
		Matrix4(2.0f, 0.0f, 0.0f, 4.0f, 
                0.0f, 2.0f, 0.0f, 3.0f, 
                0.0f, 0.0f, 2.0f, 0.0f, 
                0.0f, 0.0f, 0.0f, 1.0f);
    // clang-format on
    Vector4 vec1 = Vector4(8.0f, 6.0f, 2.0f, 1.0f);
    Vector4 vec = mat * vec1;
	testEqual(20.0f, vec[0], "First element doesn't match!");
	testEqual(15.0f, vec[1], "Second element doesn't match!");
	testEqual(4.0f, vec[2], "Third element doesn't match!");
	testEqual(1.0f, vec[3], "Fourth element doesn't match!");
}
