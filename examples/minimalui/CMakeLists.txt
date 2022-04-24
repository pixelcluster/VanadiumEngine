cmake_minimum_required (VERSION 3.8)

project ("VanadiumEngine")

file(GLOB_RECURSE CPP_SOURCES CONFIGURE_DEPENDS 
	"${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS
	"${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp")

add_executable(MinimalUI ${CPP_SOURCES} ${HEADERS})

target_include_directories(MinimalUI PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(MinimalUI PRIVATE VanadiumEngine)

vanadium_init_vcp_project()
vanadium_compile_vcp_shaders(MinimalUI)
vanadium_generate_fontcfg(${CMAKE_CURRENT_SOURCE_DIR}/fonts/fontcfg.json ./fonts.fcfg MinimalUI)