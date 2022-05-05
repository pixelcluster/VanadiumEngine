#pragma once

#include <array>
#include <string_view>

using TestFunction = void (*)();

struct FunctionEntry {
	std::string_view name;
	TestFunction function;
};

void testMatrixConstructor();
void testMatrixMultiplication();
void testMatrixVectorMultiplication();

static constexpr std::array<FunctionEntry, 3> testFunctions = {
	FunctionEntry{ "MatrixConstructor", testMatrixConstructor },
	FunctionEntry{ "MatrixMultiplication", testMatrixMultiplication },
	FunctionEntry{ "MatrixVectorMultiplication", testMatrixVectorMultiplication }
};