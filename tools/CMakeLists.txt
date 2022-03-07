cmake_minimum_required (VERSION 3.16)

project ("VanadiumEngine")

#FormatMatchTable

file(GLOB CPP_SOURCES CONFIG_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/EnumMatchTable/*.cpp")

add_executable(EnumMatchTable ${CPP_SOURCES})
target_link_libraries(EnumMatchTable tinyxml2)

#vcp

file(GLOB CPP_SOURCES CONFIG_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/vcp/src/*.cpp")

find_package(Vulkan REQUIRED FATAL_ERROR)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/generated_include)
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/generated_include/EnumMatchTable.hpp 
				   COMMAND EnumMatchTable 
				   ARGS "$ENV{VULKAN_SDK}/share/vulkan/registry/vk.xml" 
				   WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/generated_include 
				   DEPENDS EnumMatchTable)

add_executable(vcp ${CPP_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/generated_include/EnumMatchTable.hpp)
target_include_directories(vcp PUBLIC ${Vulkan_INCLUDE_DIRS} "${CMAKE_CURRENT_SOURCE_DIR}/vcp/include" ${CMAKE_CURRENT_BINARY_DIR}/generated_include)
if(MSVC)
	target_compile_options(vcp PUBLIC "/bigobj")
endif()
target_link_libraries(vcp jsoncpp_static)