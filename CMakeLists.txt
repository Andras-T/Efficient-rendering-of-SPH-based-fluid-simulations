cmake_minimum_required(VERSION 3.25)

project(SphFluid VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
foreach(config ${CMAKE_CONFIGURATION_TYPES})
  string(TOUPPER ${config} config)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${config} ${CMAKE_BINARY_DIR}/bin)
endforeach()

option(SPH_BUILD_TESTS "Build the test suites" ${PROJECT_IS_TOP_LEVEL})
option(SPH_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(SPH_WRITE_GOLDENS "Rewrite tests/goldens from GoldenImageTest" OFF)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
include(Warnings)
include(Dependencies)
include(Shaders)

add_subdirectory(shaders)
add_subdirectory(src)

if(SPH_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
