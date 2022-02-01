cmake_minimum_required (VERSION 3.8)

project ("VanadiumEngine")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB_RECURSE CPP_SOURCES CONFIGURE_DEPENDS 
	"${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS
	"${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp")

file(GLOB SHADERS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/shaders")

foreach(SHADER IN LISTS SHADERS)
	get_filename_component(FILENAME ${SHADER} NAME_WLE)
	get_filename_component(EXTENSION ${SHADER} LAST_EXT)
	get_filename_component(SOURCEPATH ${SHADER} ABSOLUTE)

	string(SUBSTRING ${EXTENSION} 1 -1 EXTENSION)

	list(APPEND SHADER_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/depend-dummy-${FILENAME}.${EXTENSION}")
	add_custom_command(OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/shaders/depend-dummy-${FILENAME}.${EXTENSION}"
					   COMMAND "glslc" ARGS "-O" "--target-env=vulkan1.0" "-o" "${CMAKE_CURRENT_BINARY_DIR}/shaders/${FILENAME}-${EXTENSION}.spv" ${SOURCEPATH}
					   DEPENDS ${SOURCEPATH})
endforeach()

add_executable(ExampleHelloWorld ${CPP_SOURCES} ${HEADERS})

target_include_directories(ExampleHelloWorld PUBLIC "include")
target_link_libraries(ExampleHelloWorld VanadiumEngine)

add_custom_target(shaders DEPENDS ${SHADER_DEPENDS})

add_dependencies(ExampleHelloWorld shaders)