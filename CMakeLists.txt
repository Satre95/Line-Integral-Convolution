cmake_minimum_required(VERSION 3.8)
project(LIC-Cinder CXX)
find_package(Threads REQUIRED)

set(CMAKE_CXX_STANDARD 14) # C++14 beacuse it's 2018...
set(CMAKE_CXX_STANDARD_REQUIRED ON) #...is required...
set(CMAKE_CXX_EXTENSIONS OFF) #...without compiler extensions like gnu++11
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_MACOSX_RPATH 1)
set(EXTERNALS_DIR ${CMAKE_SOURCE_DIR}/external)
set(CMAKE_BUILD_TYPE DEBUG)
set(THREADS_PREFER_PTHREAD ON)

# Check if the user has provided a path to their local cinder installation.
if(CINDER_PATH)	
	include("${CINDER_PATH}/proj/cmake/modules/cinderMakeApp.cmake")
	message( STATUS "Cinder path set to ${CINDER_PATH}")
else()
	message(FATAL_ERROR "Path to Cinder not provided. Please define a variable CINDER_PATH with a path to root Cinder directory.")
endif()

