cmake_minimum_required(VERSION 3.6.2)
project(agio)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

find_package(PythonLibs 3.5)
include_directories(${PYTHON_INCLUDE_DIRS})
include_directories(${CMAKE_SOURCE_DIR}/matplotlib-cpp)
include_directories(${CMAKE_SOURCE_DIR}/bullet3/src)
include_directories(${CMAKE_SOURCE_DIR}/FPlusPlus/FPlusPlus)
include_directories(${CMAKE_SOURCE_DIR}/NEAT/include)

file(GLOB neat_sources NEAT/src/*.cpp)
list(FILTER neat_sources EXCLUDE REGEX ".*neatmain.cpp$")
list(FILTER neat_sources EXCLUDE REGEX ".*experiments.cpp$")
set(project_sources 
	src/Tests/test.cpp
	src/Tests/neat_test.cpp
	src/Utils/Utils.cpp
	src/Evolution/Individual.cpp
	src/Evolution/Population.cpp
	src/Tests/PreyPredator/PreyPredator.cpp
	${neat_sources}
)

add_executable(agio ${project_sources} )

SET(BULLET2_MULTITHREADING OFF CACHE BOOL "")
SET(BUILD_CPU_DEMOS OFF CACHE BOOL "")
SET(BUILD_BULLET2_DEMOS OFF CACHE BOOL "")
SET(USE_GLUT OFF CACHE BOOL "")
SET(BUILD_UNIT_TESTS OFF CACHE BOOL "")
add_subdirectory(bullet3)

target_link_libraries(agio ${PYTHON_LIBRARIES})
target_link_libraries(agio BulletDynamics BulletCollision LinearMath)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT agio)

if (WIN32)
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /std:c++latest /MT -DWITHOUT_NUMPY")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /std:c++latest /MTd -DWITHOUT_NUMPY")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /std:c++latest /MT -DWITHOUT_NUMPY")
else ()
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -std=c++1z -DWITHOUT_NUMPY")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -std=c++1z -DWITHOUT_NUMPY")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -std=c++1z -DWITHOUT_NUMPY")
endif ()