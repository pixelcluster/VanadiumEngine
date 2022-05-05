cmake_minimum_required (VERSION 3.8)

project ("VanadiumEngine")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(SphereIntersectionTest sphere_int.cpp AtmosphereParamsCommon.hpp)
target_include_directories(SphereIntersectionTest PUBLIC ${CMAKE_SOURCE_DIR}/tests/framework)
target_link_libraries(SphereIntersectionTest glm::glm TestCommon)

add_executable(OpticalDepthTest opticaldepth_int.cpp AtmosphereParamsCommon.hpp)
target_include_directories(OpticalDepthTest PUBLIC ${CMAKE_SOURCE_DIR}/tests/framework)
target_link_libraries(OpticalDepthTest glm::glm TestCommon)

add_executable(ExtinctionTest height_exp.cpp AtmosphereParamsCommon.hpp)
target_include_directories(ExtinctionTest PUBLIC ${CMAKE_SOURCE_DIR}/tests/framework)
target_link_libraries(ExtinctionTest glm::glm TestCommon)

add_test(NAME "sphere_intersection" COMMAND SphereIntersectionTest)
add_test(NAME "optical_depth" COMMAND OpticalDepthTest)
add_test(NAME "extinction" COMMAND ExtinctionTest)