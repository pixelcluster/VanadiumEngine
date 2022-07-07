cmake_minimum_required (VERSION 3.16)

project ("VanadiumEngine")

#EnumMatchTable

file(GLOB CPP_SOURCES CONFIG_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/EnumMatchTable/src/*.cpp")

add_executable(EnumMatchTable ${CPP_SOURCES})
target_include_directories(EnumMatchTable PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/EnumMatchTable/include"
												 "${CMAKE_SOURCE_DIR}/include")
target_link_libraries(EnumMatchTable tinyxml2)

if(NOT MSVC)
	target_compile_options(EnumMatchTable PRIVATE "-Wall")
endif()
#Font configuration file generator

file(GLOB CPP_SOURCES CONFIG_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/fontcfg/src/*.cpp")

add_executable(fontcfg ${CPP_SOURCES})
target_link_libraries(fontcfg jsoncpp_static)
target_include_directories(fontcfg PUBLIC 
	"${CMAKE_SOURCE_DIR}/include"
	"${CMAKE_CURRENT_SOURCE_DIR}/common/include" 
	)

if(NOT MSVC)
	target_compile_options(fontcfg PRIVATE "-Wall")
endif()

function(vanadium_generate_fontcfg JSON_PATH OUTFILE TARGETNAME)
	add_custom_target(fonts_${TARGETNAME} fontcfg ${JSON_PATH} "-o" "${CMAKE_CURRENT_BINARY_DIR}/${OUTFILE}")
	add_dependencies(${TARGETNAME} fonts_${TARGETNAME})
endfunction()

#Line breaking rules, aided by automatically generated character classifications
if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/UnicodeCharacterAssoc.txt")
	file(DOWNLOAD https://www.unicode.org/Public/UCD/latest/ucd/LineBreak.txt "${CMAKE_CURRENT_BINARY_DIR}/UnicodeCharacterAssoc.txt")
endif()
if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/UnicodeEastAsianWidth.txt")
	file(DOWNLOAD http://www.unicode.org/Public/UCD/latest/ucd/EastAsianWidth.txt "${CMAKE_CURRENT_BINARY_DIR}/UnicodeEastAsianWidth.txt")
endif()
if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/UnicodeExtendedPictographic.txt")
	file(DOWNLOAD https://www.unicode.org/Public/UCD/latest/ucd/emoji/emoji-data.txt "${CMAKE_CURRENT_BINARY_DIR}/UnicodeExtendedPictographic.txt")
endif()

file(GLOB CPP_SOURCES CONFIG_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/LineBreakRules/src/*.cpp")
add_executable(UnicodeLineBreak ${CPP_SOURCES})
target_include_directories(UnicodeLineBreak PUBLIC 
						   "${CMAKE_SOURCE_DIR}/include")

set(CHARACTERGROUP_FILE ${CMAKE_CURRENT_BINARY_DIR}/generated_include/CharacterGroup.hpp PARENT_SCOPE)

#vcp, Vanadium Compiled Pipeline generator

file(GLOB CPP_SOURCES CONFIG_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/vcp/src/*.cpp")

find_package(Vulkan REQUIRED FATAL_ERROR)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/generated_include)

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/generated_include/EnumMatchTable.hpp 
				   COMMAND EnumMatchTable 
				   ARGS "$ENV{VULKAN_SDK}/share/vulkan/registry/vk.xml" 
				   WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/generated_include 
				   DEPENDS EnumMatchTable)

add_executable(vcp ${CPP_SOURCES} "${CMAKE_SOURCE_DIR}/dependencies/spirv-reflect/spirv_reflect.c" ${CMAKE_CURRENT_BINARY_DIR}/generated_include/EnumMatchTable.hpp)
#target_compile_definitions(vcp PRIVATE "VCP_EXPORTER_DIAGNOSTICS")

if(NOT MSVC)
	target_compile_options(vcp PRIVATE "-Wall")
endif()

set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/generated_include/EnumMatchTable.hpp PROPERTIES GENERATED TRUE)

target_include_directories(vcp PUBLIC 
	${Vulkan_INCLUDE_DIRS} 
	"${CMAKE_CURRENT_SOURCE_DIR}/vcp/include" 
	"${CMAKE_CURRENT_SOURCE_DIR}/common/include" 
	"${CMAKE_SOURCE_DIR}/include" 
	"${CMAKE_SOURCE_DIR}/dependencies/spirv-reflect" 
	"${CMAKE_CURRENT_BINARY_DIR}/generated_include")

if(MSVC)
	target_compile_options(vcp PUBLIC "/bigobj")
endif()
target_link_libraries(vcp jsoncpp_static fmt)

function(vanadium_init_vcp)
	set(VANADIUM_STD_VCP_SHADERS "" PARENT_SCOPE)
endfunction()

function(vanadium_init_vcp_project)
	set(VANADIUM_VCP_SHADERS "" PARENT_SCOPE)
	set(VANADIUM_VCP_FILE_PATH "./shaders.vcp" PARENT_SCOPE)
endfunction()

macro(vanadium_add_vcp_shader FILE)
	get_filename_component(FULLPATH ${FILE} ABSOLUTE)
	list(APPEND VANADIUM_VCP_SHADERS ${FULLPATH})
endmacro()

macro(vanadium_add_std_vcp_shader FILE)
	get_filename_component(FULLPATH ${FILE} ABSOLUTE)
	list(APPEND VANADIUM_STD_VCP_SHADERS ${FULLPATH})
endmacro()

function(vanadium_compile_vcp_shaders TARGETNAME)
	add_custom_target(shaders_${TARGETNAME} vcp ${VANADIUM_STD_VCP_SHADERS} ${VANADIUM_VCP_SHADERS} "-o" "${CMAKE_CURRENT_BINARY_DIR}/${VANADIUM_VCP_FILE_PATH}")
	add_dependencies(${TARGETNAME} shaders_${TARGETNAME})
endfunction()