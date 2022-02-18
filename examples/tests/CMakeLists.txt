cmake_minimum_required (VERSION 3.8)

project ("VanadiumEngine")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_library(TestCommon INTERFACE)
target_include_directories(TestCommon INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

add_subdirectory(atmosphere)