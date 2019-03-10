cmake_minimum_required(VERSION 3.6.2)
project(agio)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

add_subdirectory(boost-cmake)

include_directories(${CMAKE_SOURCE_DIR}/matplotlib-cpp)
include_directories(${CMAKE_SOURCE_DIR}/FPlusPlus/FPlusPlus)
include_directories(${CMAKE_SOURCE_DIR}/NEAT/include)

file(GLOB neat_sources NEAT/src/*.cpp)
list(FILTER neat_sources EXCLUDE REGEX ".*neatmain.cpp$")
list(FILTER neat_sources EXCLUDE REGEX ".*experiments.cpp$")
set(project_sources 
	src/Utils/Utils.cpp
	src/Evolution/Individual.cpp
	src/Evolution/Population.cpp
	src/Evolution/Globals.cpp
	src/Serialization/SIndividual.cpp
	src/Serialization/SNetwork.cpp
	src/Serialization/SRegistry.cpp
	src/Evolution/MorphologyTag.cpp
	src/Core/Config.cpp
	src/Utils/ConsoleRenderer.cpp
	src/Utils/SFMLRenderer.cpp
	${neat_sources})

add_library(agio_lib STATIC ${project_sources})

target_link_libraries(agio_lib Boost::serialization) # While libraries aren't linked, cmake seems to need the dependency

if (WIN32)
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /std:c++17 -DWITHOUT_NUMPY /W3 /Od")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /std:c++17 -DWITHOUT_NUMPY /W3")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /std:c++17 -DWITHOUT_NUMPY /Ox /GL /W3 /Zi")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LTCG /DEBUG /OPT:REF")
else ()
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -std=c++17 -DWITHOUT_NUMPY -O0")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -std=c++17 -DWITHOUT_NUMPY")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -std=c++17 -DWITHOUT_NUMPY -O3")
endif ()

option(build_tests "Build the AGIO demo tests after building the library" ON)
if (build_tests)
	find_package(PythonLibs 3.5)
	include_directories(${PYTHON_INCLUDE_DIRS})

	add_executable(prey_predator
			src/Tests/PreyPredator/main.cpp
			src/Tests/PreyPredator/PreyPredatorEvolution.cpp
			src/Tests/PreyPredator/PreyPredator.cpp
			src/Tests/PreyPredator/PublicInterfaceImpl.cpp)
	
	add_executable(experiment_3d
		src/Tests/3DExperiment/3DExperiment.cpp
		src/Tests/3DExperiment/main.cpp)

	set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
	if (WIN32)
		file(GLOB SFML_LIBRARIES libs_windows/sfml/lib/*.lib)
		set(SFML_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/libs_windows/sfml/include)
	else ()
		find_package(SFML COMPONENTS graphics window system REQUIRED)	
	endif ()

	include_directories(${SFML_INCLUDE_DIR})

	add_executable(diversity_plot
			src/Tests/DiversityPlot/main.cpp
			src/Tests/DiversityPlot/DiversityPlot.cpp
			src/Tests/DiversityPlot/DiversityPlotEvolution.cpp
			src/Tests/DiversityPlot/Character.cpp
			src/Tests/DiversityPlot/AnimatedSprite.cpp
			src/Tests/DiversityPlot/Animation.cpp
			src/Tests/DiversityPlot/PublicInterfaceImpl.cpp)

	add_executable(human_test
			src/Tests/DiversityPlot/HumanTest.cpp
			src/Tests/DiversityPlot/DiversityPlot.cpp
			src/Tests/DiversityPlot/DiversityPlotEvolution.cpp
			src/Tests/DiversityPlot/Character.cpp
			src/Tests/DiversityPlot/AnimatedSprite.cpp
			src/Tests/DiversityPlot/Animation.cpp
			src/Tests/DiversityPlot/PublicInterfaceImpl.cpp)
	
	add_executable(perf_test
			src/Tests/DiversityPlot/PerfTest.cpp
			src/Tests/DiversityPlot/DiversityPlot.cpp
			src/Tests/DiversityPlot/PublicInterfaceImpl.cpp)
	
	add_executable(memory_test
			src/Tests/DiversityPlot/MemoryTest.cpp
			src/Tests/DiversityPlot/DiversityPlot.cpp
			src/Tests/DiversityPlot/PublicInterfaceImpl.cpp)
			
	target_link_libraries(memory_test
			agio_lib
			${PYTHON_LIBRARIES}
			Boost::serialization
			${SFML_LIBRARIES})

	target_link_libraries(perf_test
			agio_lib
			${PYTHON_LIBRARIES}
			Boost::serialization
			${SFML_LIBRARIES})

	target_link_libraries(human_test
			agio_lib
			${PYTHON_LIBRARIES}
			Boost::serialization
			${SFML_LIBRARIES})

	target_link_libraries(diversity_plot
			agio_lib
			${PYTHON_LIBRARIES}
			Boost::serialization
			${SFML_LIBRARIES})

	target_link_libraries(prey_predator
		agio_lib
		${PYTHON_LIBRARIES}
		Boost::serialization
		${SFML_LIBRARIES})

	target_link_libraries(experiment_3d
		agio_lib
		${PYTHON_LIBRARIES}
		Boost::serialization
		${SFML_LIBRARIES})

	if (WIN32)
		# Extract the python DLL path from the lib path
		set(PYTHON_DLL ${PYTHON_LIBRARIES})

		string(REGEX REPLACE "optimized(.+)\\/libs\\/(.+).lib.*" "\\1" REGEX_OUTPUT ${PYTHON_DLL})
		set(PYTHON_DLL ${REGEX_OUTPUT})

		string(REGEX REPLACE "[.]lib.*" ".dll" REGEX_OUTPUT ${PYTHON_DLL})
		set(PYTHON_DLL ${REGEX_OUTPUT})

		string(REGEX REPLACE "libs\\/" "" REGEX_OUTPUT ${PYTHON_DLL})
		set(PYTHON_DLL ${REGEX_OUTPUT})

        add_custom_command(
            TARGET agio_lib
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ${PYTHON_DLL} ${CMAKE_SOURCE_DIR}/bin
            COMMENT "Copying the Python DLL to the binaries directory.")
	endif()
endif()
