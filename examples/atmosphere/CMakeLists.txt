cmake_minimum_required (VERSION 3.8)

project ("VanadiumEngine")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB_RECURSE CPP_SOURCES CONFIGURE_DEPENDS 
	"${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS
	"${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/shaders")

if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/resources/earth_tex.jpg")
	file(DOWNLOAD "https://eoimages.gsfc.nasa.gov/images/imagerecords/74000/74218/world.200412.3x5400x2700.jpg" "${CMAKE_CURRENT_BINARY_DIR}/resources/earth_tex.jpg" SHOW_PROGRESS)
endif()

if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/resources/earth_bathymetry.jpg")
	file(DOWNLOAD "http://planetpixelemporium.com/download/download.php?8081/earthspec10k.jpg" "${CMAKE_CURRENT_BINARY_DIR}/resources/earth_bathymetry.jpg" SHOW_PROGRESS)
endif()

add_executable(ExampleAtmosphere ${CPP_SOURCES} ${HEADERS})

target_include_directories(ExampleAtmosphere PUBLIC "include" "${CMAKE_SOURCE_DIR}/examples/dependencies/stb")
target_link_libraries(ExampleAtmosphere VanadiumEngine glm)

add_custom_target(atmosphere_shaders DEPENDS ${SHADER_DEPENDS})

add_dependencies(ExampleAtmosphere atmosphere_shaders)

#vanadium_init_vcp_project()
#vanadium_add_vcp_shader("shaders/transmittance.json")
#vanadium_add_vcp_shader("shaders/planet.json")
#vanadium_compile_vcp_shaders(ExampleAtmosphere)