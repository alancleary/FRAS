cmake_minimum_required(VERSION 3.15 FATAL_ERROR)

# set the build type to release if not specified
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# setup the CMake project
project("CFG-AMT"
        VERSION 0.0.1
        DESCRIPTION "Represents a context-free grammar as an array mapped trie"
        LANGUAGES CXX
)

# set the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# set compiler flags
set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

# locate all source files
file(GLOB_RECURSE SOURCES src/*.cpp)

# compile the sources into an executable
add_executable(${PROJECT_NAME} ${SOURCES})

# specify include directories
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
