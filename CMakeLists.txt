# SPDX-License-Identifier: GPL-3.0-or-later
cmake_minimum_required(VERSION 3.20)

project(BackyardRacer VERSION 0.1.0 LANGUAGES C CXX)

include(FetchContent)

# Common is the canonical shared implementation. Pin the exact reviewed release
# commit so a future Common main change cannot silently change this game build.
set(INFILTRATR_COMMON_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(INFILTRATR_COMMON_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)
set(INFILTRATR_COMMON_BUILD_SHARED OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    InfiltratrCommon
    GIT_REPOSITORY https://github.com/Infiltrator-Projects/Infiltrator-Libraries.git
    GIT_TAG e9b747c7c0530590fbba1f57788e046203ef8c4e
)
FetchContent_MakeAvailable(InfiltratrCommon)

find_package(X11 REQUIRED)

add_executable(backyard-racer src/main.cpp)
target_compile_features(backyard-racer PRIVATE cxx_std_20)
set_target_properties(backyard-racer PROPERTIES CXX_EXTENSIONS OFF)
target_link_libraries(backyard-racer PRIVATE InfiltratrCommon::Portable X11::X11)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_compile_options(backyard-racer PRIVATE -Wall -Wextra -Wpedantic)
endif()
