cmake_minimum_required (VERSION 3.16)

project ("VanadiumEngine")

#EnumMatchTable

file(GLOB CPP_SOURCES CONFIG_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/EnumMatchTable/src/*.cpp")

add_executable(EnumMatchTable ${CPP_SOURCES})
target_include_directories(EnumMatchTable PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/EnumMatchTable/include")
target_link_libraries(EnumMatchTable tinyxml2)

#vcp

file(GLOB CPP_SOURCES CONFIG_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/vcp/src/*.cpp")

find_package(Vulkan REQUIRED FATAL_ERROR)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/generated_include)

if(EXISTS "/usr/share/vulkan/registry/vk.xml")
	set(SDK_PATH "/usr")
else()
	set(SDK_PATH $ENV{VULKAN_SDK})
endif()

add_executable(vcp ${CPP_SOURCES} "${CMAKE_SOURCE_DIR}/dependencies/spirv-reflect/spirv_reflect.c" ${CMAKE_CURRENT_BINARY_DIR}/generated_include/EnumMatchTable.hpp)

target_include_directories(vcp PUBLIC 
	${Vulkan_INCLUDE_DIRS} 
	"${CMAKE_CURRENT_SOURCE_DIR}/vcp/include" 
	"${CMAKE_SOURCE_DIR}/include" 
	"${CMAKE_SOURCE_DIR}/dependencies/spirv-reflect" 
	"${CMAKE_CURRENT_BINARY_DIR}/generated_include")

if(MSVC)
	target_compile_options(vcp PUBLIC "/bigobj")
endif()
target_link_libraries(vcp jsoncpp_static)