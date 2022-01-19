cmake_minimum_required (VERSION 3.8)

project ("VanadiumEngine")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_subdirectory(dependencies/glfw)
add_subdirectory(dependencies/volk)

if(UNIX AND NOT APPLE)
	option(USE_WAYLAND "Use Wayland instead of X11")
	if(USE_WAYLAND)
		set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WAYLAND_KHR)
		set(GLFW_USE_WAYLAND TRUE)
	endif()
elseif(WIN32)
	set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WIN32_KHR)
endif()

if("${CMAKE_BUILD_TYPE}" MATCHES "Debug")
	set(VANADIUM_DEBUG true)
else()
	set(VANADIUM_DEBUG false)
endif()

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/include/VDebug.hpp.in" "${CMAKE_CURRENT_SOURCE_DIR}/include/VDebug.hpp")

file(GLOB_RECURSE CPP_SOURCES CONFIGURE_DEPENDS 
	"${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS
	"${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp")

find_package(Vulkan REQUIRED FATAL_ERROR)

add_library(VanadiumEngine ${CPP_SOURCES} ${HEADERS})

target_include_directories(VanadiumEngine PUBLIC "include" ${Vulkan_INCLUDE_DIRS})
target_link_libraries(VanadiumEngine volk::volk glfw)

add_subdirectory(examples)