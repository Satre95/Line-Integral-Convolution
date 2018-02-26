cmake_minimum_required(VERSION 3.8)
project(Vortex-Simulation CXX)
find_package(Threads REQUIRED)

set(CMAKE_BUILD_TYPE DEBUG)

set(CMAKE_CXX_STANDARD 14) # C++14 beacuse it's 2018...
set(CMAKE_CXX_STANDARD_REQUIRED ON) #...is required...
set(CMAKE_CXX_EXTENSIONS OFF) #...without compiler extensions like gnu++11
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_MACOSX_RPATH 1)
set(EXTERNALS_DIR ${CMAKE_SOURCE_DIR}/external)
set(THREADS_PREFER_PTHREAD ON)
set(CMAKE_VERBOSE_MAKEFILE ON )

# Setup dependencies
add_subdirectory(external)

# Setup source and header files.
add_subdirectory(source)

# Build the app config
add_executable(VortexSimulation WIN32 ${SOURCE_FILES} ${HEADER_FILES})
target_link_libraries(VortexSimulation PRIVATE openFrameworks)
