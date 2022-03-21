cmake_minimum_required (VERSION 3.16)

project ("VanadiumEngine")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

option(USE_TESTS "Build and run tests" ON)


if(${USE_TESTS}) 
	enable_testing()
endif()

if(UNIX AND NOT APPLE)
	option(USE_WAYLAND "Use Wayland instead of X11" ON)
	if(USE_WAYLAND)
		set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WAYLAND_KHR)
		add_compile_definitions(VK_USE_PLATFORM_WAYLAND_KHR)
		set(GLFW_BUILD_X11 FALSE)
		set(GLFW_USE_WAYLAND TRUE)
		set(GLFW_BUILD_WAYLAND TRUE)
	else()
		set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_XLIB_KHR)
		add_compile_definitions(VK_USE_PLATFORM_XLIB_KHR)
	endif()
elseif(WIN32)
	set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WIN32_KHR)
	add_compile_definitions(VK_USE_PLATFORM_WIN32_KHR NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

add_subdirectory(dependencies/glfw)
add_subdirectory(dependencies/volk)
add_subdirectory(dependencies/tinyxml EXCLUDE_FROM_ALL)
add_subdirectory(dependencies/jsoncpp EXCLUDE_FROM_ALL)
add_subdirectory(dependencies/robin-hood)

add_subdirectory(tools)


if("${CMAKE_BUILD_TYPE}" MATCHES "Debug")
	set(VANADIUM_DEBUG true CACHE BOOL "Enable debug utilities")
else()
	set(VANADIUM_DEBUG false CACHE BOOL "Enable debug utilities")
endif()

set(VANADIUM_GPU_DEBUG false CACHE BOOL "Enable debug utilities for GPU modules")

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/include/Debug.hpp.in" "${CMAKE_CURRENT_SOURCE_DIR}/include/Debug.hpp")

file(GLOB_RECURSE CPP_SOURCES CONFIGURE_DEPENDS 
	"${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS
	"${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp")

find_package(Vulkan REQUIRED FATAL_ERROR)

add_library(VanadiumEngine ${CPP_SOURCES} ${HEADERS})

target_precompile_headers(VanadiumEngine PRIVATE "include/Engine.hpp")

target_include_directories(VanadiumEngine PUBLIC "include" ${Vulkan_INCLUDE_DIRS} "dependencies/stb")
target_link_libraries(VanadiumEngine volk::volk glfw robin_hood)

add_subdirectory(examples)