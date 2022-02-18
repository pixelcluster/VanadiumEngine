cmake_minimum_required (VERSION 3.8)

project ("VanadiumEngine")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_subdirectory(dependencies/glm)
add_subdirectory(helloworld)
add_subdirectory(atmosphere)
if(${USE_TESTS})
	add_subdirectory(tests)
endif()