cmake_minimum_required(VERSION 3.15 FATAL_ERROR)

# set the build type to release if not specified
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# setup the CMake project
project("fras"
        VERSION 0.0.1
        DESCRIPTION "Represents a context-free grammar as an array mapped trie"
        LANGUAGES CXX
)

# set the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# install external libraries
include(FetchContent)

# SDSL
FetchContent_Declare(
  sdsl
  GIT_REPOSITORY https://github.com/xxsds/sdsl-lite.git
  GIT_TAG v3.0.3
  GIT_SHALLOW TRUE
  GIT_PROGRESS TRUE
  CONFIGURE_COMMAND ""
  BUILD_COMMAND "")

#FetchContent_MakeAvailable(sdsl)

FetchContent_GetProperties(sdsl)
if(NOT sdsl_POPULATED)
  FetchContent_Populate(sdsl)
endif()


# set compiler flags
#set(CMAKE_CXX_FLAGS "-Wall -Wextra")
#set(CMAKE_CXX_FLAGS_DEBUG "-g")
#set(CMAKE_CXX_FLAGS_RELEASE "-O3")

# locate all source files
file(GLOB_RECURSE SOURCES src/*.cpp)

# compile the sources into an executable
add_executable(${PROJECT_NAME} ${SOURCES})

# specify include directories
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
# TODO: this should be done with target_include_directories
#include_directories("${sdsl_SOURCE_DIR}/include")

# link the libraries
target_include_directories(${PROJECT_NAME} PRIVATE ${sdsl_SOURCE_DIR}/include)
