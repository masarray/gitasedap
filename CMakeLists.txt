cmake_minimum_required(VERSION 3.25)

project(
  GitaSedap
  VERSION 0.1.0
  DESCRIPTION "Low-latency acoustic guitar body and polish processor"
  LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(GITASEDAP_BUILD_PLUGIN "Build the VST3 plug-in target" ON)
option(GITASEDAP_BUILD_TESTS "Build unit and invariant tests" ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(Warnings)

add_library(gitasedap_core INTERFACE)
target_include_directories(
  gitasedap_core
  INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_compile_features(gitasedap_core INTERFACE cxx_std_20)

add_subdirectory(src/runtime)

if(GITASEDAP_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

if(GITASEDAP_BUILD_PLUGIN)
  include(Dependencies)
  add_subdirectory(src/plugin)
endif()
