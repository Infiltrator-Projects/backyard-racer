# SPDX-License-Identifier: GPL-3.0-or-later
cmake_minimum_required(VERSION 3.20)

project(BackyardRacer VERSION 0.6.0 LANGUAGES C CXX)

include(FetchContent)

# Common owns generic timing, durable POSIX publication and software-pixel
# surface operations. Pin the exact reviewed release so gameplay, renderer and
# host adapter all consume the same immutable implementation.
set(INFILTRATR_COMMON_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(INFILTRATR_COMMON_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)
set(INFILTRATR_COMMON_BUILD_SHARED OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    InfiltratrCommon
    GIT_REPOSITORY https://github.com/Infiltrator-Projects/Infiltrator-Libraries.git
    GIT_TAG 9b6e92ef135f7e4bc0af809975a619aa20a2eaa3
)
FetchContent_MakeAvailable(InfiltratrCommon)

find_package(X11 REQUIRED)
find_package(ZLIB REQUIRED)

add_library(backyard-gameplay STATIC
    src/gameplay.cpp
    src/vehicle_data.cpp
    src/race_session.cpp)
target_include_directories(backyard-gameplay PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_features(backyard-gameplay PUBLIC cxx_std_20)
set_target_properties(backyard-gameplay PROPERTIES CXX_EXTENSIONS OFF)
target_link_libraries(backyard-gameplay PUBLIC InfiltratrCommon::Common)

# Platform-neutral presentation core. It owns every player-visible pixel and
# deliberately has no X11 dependency. X11 is confined to the host adapter.
add_library(backyard-pixel-core STATIC
    src/pixel_app.cpp)
target_include_directories(backyard-pixel-core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_features(backyard-pixel-core PUBLIC cxx_std_20)
set_target_properties(backyard-pixel-core PROPERTIES CXX_EXTENSIONS OFF)
target_link_libraries(backyard-pixel-core PUBLIC
    backyard-gameplay
    ZLIB::ZLIB)

add_executable(backyard-racer
    src/main_pixel.cpp
    src/platform_x11.cpp)
target_compile_features(backyard-racer PRIVATE cxx_std_20)
set_target_properties(backyard-racer PROPERTIES CXX_EXTENSIONS OFF)
target_link_libraries(backyard-racer PRIVATE
    backyard-pixel-core
    X11::X11)

add_executable(backyard-gameplay-test tests/gameplay_test.cpp)
target_link_libraries(backyard-gameplay-test PRIVATE backyard-gameplay)

add_executable(backyard-race-session-test tests/race_session_test.cpp)
target_link_libraries(backyard-race-session-test PRIVATE backyard-gameplay)

add_executable(backyard-pixel-core-test tests/pixel_core_test.cpp)
target_link_libraries(backyard-pixel-core-test PRIVATE backyard-pixel-core)

add_executable(backyard-paint-state-test tests/paint_state_test.cpp)
target_link_libraries(backyard-paint-state-test PRIVATE backyard-gameplay)

enable_testing()
add_test(NAME gameplay-smoke COMMAND backyard-gameplay-test)
add_test(NAME race-session-common-timing COMMAND backyard-race-session-test)
add_test(NAME pixel-core-no-x11 COMMAND backyard-pixel-core-test)
add_test(NAME paint-state-persistence COMMAND backyard-paint-state-test)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    foreach(target
            backyard-gameplay
            backyard-pixel-core
            backyard-racer
            backyard-gameplay-test
            backyard-race-session-test
            backyard-pixel-core-test
            backyard-paint-state-test)
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endforeach()
endif()
