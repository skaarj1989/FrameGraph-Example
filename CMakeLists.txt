cmake_minimum_required(VERSION 3.20)

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE
    "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
    CACHE STRING "")
endif()

message(STATUS "TOOLCHAIN: ${CMAKE_TOOLCHAIN_FILE}")

project(FrameGraphExample)

if(PROJECT_BINARY_DIR STREQUAL PROJECT_SOURCE_DIR)
  message(
    FATAL_ERROR
    "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
  )
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib") # .lib
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib") # .dll
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin") # .exe

option(TRACY_ENABLE "Enable profiler" OFF)

if(WIN32)
  message(STATUS "Build for WIN32")
  add_definitions("-DVC_EXTRALEAN" "-DWIN32_LEAN_AND_MEAN -DNOMINMAX")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  message(STATUS "Build for Linux")
  set(LINUX YES)
endif()

include(FetchContent)

FetchContent_Declare(
  glad
  GIT_REPOSITORY https://github.com/skaarj1989/glad.git
  GIT_TAG gl)
FetchContent_Declare(
  FrameGraph
  GIT_REPOSITORY https://github.com/skaarj1989/FrameGraph.git
  GIT_TAG master)
FetchContent_Declare(
  Tracy
  GIT_REPOSITORY https://github.com/wolfpld/tracy
  GIT_TAG master)

FetchContent_MakeAvailable(glad)
FetchContent_MakeAvailable(FrameGraph)
FetchContent_MakeAvailable(Tracy)

include(cmake/AddResources.cmake)
add_resources(TARGET Shaders IN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders OUT_DIR shaders)
add_resources(TARGET Assets IN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/assets OUT_DIR assets)

add_subdirectory(src)
