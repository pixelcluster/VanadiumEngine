cmake_minimum_required (VERSION 3.8)

project ("VanadiumEngine")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB_RECURSE CPP_SOURCES CONFIGURE_DEPENDS 
	"${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS
	"${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp")

add_custom_target(shaders vcp "--working-dir" "${CMAKE_CURRENT_SOURCE_DIR}/shaders" "project.json" "-o" "${CMAKE_CURRENT_BINARY_DIR}/shaders.vcp")

add_executable(ExampleHelloWorld ${CPP_SOURCES} ${HEADERS})

target_include_directories(ExampleHelloWorld PUBLIC "include")
target_link_libraries(ExampleHelloWorld VanadiumEngine)

add_dependencies(ExampleHelloWorld shaders)