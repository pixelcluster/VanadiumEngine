cmake_minimum_required (VERSION 3.16)

project ("VanadiumEngine")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)


option(USE_TESTS "Build and run tests" ON)

if(UNIX AND NOT APPLE)
	set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WAYLAND_KHR VK_USE_PLATFORM_XLIB_KHR VK_USE_PLATFORM_XCB_KHR)
	add_compile_definitions(VK_USE_PLATFORM_WAYLAND_KHR VK_USE_PLATFORM_XLIB_KHR VK_USE_PLATFORM_XCB_KHR)
	set(GLFW_BUILD_WAYLAND TRUE)
	set(GLFW_BUILD_X11 TRUE)
elseif(WIN32)
	set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WIN32_KHR)
	add_compile_definitions(VK_USE_PLATFORM_WIN32_KHR NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

set(JSONCPP_WITH_POST_BUILD_UNITTEST FALSE)
set(JSONCPP_WITH_TESTS FALSE)
set(tinyxml2_BUILD_TESTING FALSE)
set(BUILD_TESTING FALSE)

add_subdirectory(dependencies/glfw)
add_subdirectory(dependencies/volk)
add_subdirectory(dependencies/tinyxml EXCLUDE_FROM_ALL)
add_subdirectory(dependencies/jsoncpp EXCLUDE_FROM_ALL)
add_subdirectory(dependencies/robin-hood)
add_subdirectory(dependencies/fmt EXCLUDE_FROM_ALL)

find_package(TBB)

# FreeType can make use of HarfBuzz, which is compiled directly into the VanadiumEngine static lib.
# The CMake file searches for a HarfBuzz module, which we want to intercept with our own.
# See FindHarfBuzz.cmake
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}")
add_subdirectory(dependencies/freetype)

if(${USE_TESTS}) 
	enable_testing()
	add_subdirectory(tests)
endif()

add_subdirectory(tools)
vanadium_init_vcp()
include(src/ui/shaders/CMakeLists.txt)

if("${CMAKE_BUILD_TYPE}" MATCHES "Debug")
	set(VANADIUM_DEBUG true CACHE BOOL "Enable debug utilities")
else()
	set(VANADIUM_DEBUG false CACHE BOOL "Enable debug utilities")
endif()

set(VANADIUM_GPU_DEBUG true CACHE BOOL "Enable debug utilities for GPU modules")

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/include/Debug.hpp.in" "${CMAKE_CURRENT_SOURCE_DIR}/include/Debug.hpp")

file(GLOB_RECURSE CPP_SOURCES CONFIGURE_DEPENDS 
	"${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS
	"${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp")

find_package(Vulkan REQUIRED FATAL_ERROR)

add_custom_command(OUTPUT ${CHARACTERGROUP_FILE}
				   COMMAND UnicodeLineBreak
				   WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/tools
				   DEPENDS UnicodeLineBreak)

add_library(VanadiumEngine ${CPP_SOURCES} ${HEADERS} dependencies/harfbuzz/src/harfbuzz.cc ${CHARACTERGROUP_FILE})

set_source_files_properties(${CHARACTERGROUP_FILE} PROPERTIES GENERATED TRUE)

target_include_directories(VanadiumEngine PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/tools/generated_include" "include" ${Vulkan_INCLUDE_DIRS} "dependencies/stb" "dependencies/harfbuzz/src")
target_link_libraries(VanadiumEngine volk::volk fmt::fmt glfw robin_hood freetype)

if(UNIX AND NOT APPLE AND TBB_FOUND)
	target_link_libraries(VanadiumEngine tbb)
endif()

add_subdirectory(examples)

# test
vanadium_init_vcp_project()
vanadium_add_vcp_shader("examples/helloworld/shaders/project.json")
vanadium_compile_vcp_shaders(VanadiumEngine)
