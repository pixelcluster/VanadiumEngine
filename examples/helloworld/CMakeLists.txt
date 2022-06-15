cmake_minimum_required (VERSION 3.8)

project ("VanadiumEngine")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB_RECURSE CPP_SOURCES CONFIGURE_DEPENDS 
	"${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS
	"${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp")

add_executable(ExampleHelloWorld ${CPP_SOURCES} ${HEADERS})

target_include_directories(ExampleHelloWorld PUBLIC "include")
target_link_libraries(ExampleHelloWorld VanadiumEngine)

#vanadium_init_vcp_project()
#vanadium_add_vcp_shader("shaders/project.json")
#vanadium_compile_vcp_shaders(ExampleHelloWorld)