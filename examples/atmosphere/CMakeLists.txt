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

file(DOWNLOAD "https://eoimages.gsfc.nasa.gov/images/imagerecords/74000/74218/world.200412.3x5400x2700.jpg" "${CMAKE_CURRENT_BINARY_DIR}/resources/earth_tex.jpg" SHOW_PROGRESS)
file(DOWNLOAD "http://planetpixelemporium.com/download/download.php?8081/earthspec10k.jpg" "${CMAKE_CURRENT_BINARY_DIR}/resources/earth_bathymetry.jpg" SHOW_PROGRESS)

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

add_executable(ExampleAtmosphere ${CPP_SOURCES} ${HEADERS})

target_include_directories(ExampleAtmosphere PUBLIC "include" "${CMAKE_SOURCE_DIR}/examples/dependencies/stb")
target_link_libraries(ExampleAtmosphere VanadiumEngine glm)

add_custom_target(atmosphere_shaders DEPENDS ${SHADER_DEPENDS})

add_dependencies(ExampleAtmosphere atmosphere_shaders)