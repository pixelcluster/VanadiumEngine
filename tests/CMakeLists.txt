cmake_minimum_required (VERSION 3.16)

project ("VanadiumEngine")

file(GLOB_RECURSE CPP_SOURCES CONFIGURE_DEPENDS 
	"${CMAKE_CURRENT_SOURCE_DIR}/math/src/*.cpp")

add_executable(MathTests ${CPP_SOURCES})
target_include_directories(MathTests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/framework ${CMAKE_CURRENT_SOURCE_DIR}/math/include ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(MathTests fmt::fmt)

add_test(NAME MatrixConstructor COMMAND MathTests "MatrixConstructor")
add_test(NAME MatrixMultiplication COMMAND MathTests "MatrixMultiplication")
add_test(NAME MatrixVectorMultiplication COMMAND MathTests "MatrixVectorMultiplication")