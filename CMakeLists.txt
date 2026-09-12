# SPDX-License-Identifier: GPL-3.0-or-later
cmake_minimum_required(VERSION 3.20)

project(BackyardRacer VERSION 0.6.0 LANGUAGES C CXX)

include(FetchContent)

# Common is the canonical shared implementation. Pin the exact immutable
# v1.16.0 release commit so the game uses the reviewed fixed-step scheduler and
# future Common main changes cannot silently alter this build.
set(INFILTRATR_COMMON_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(INFILTRATR_COMMON_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)
set(INFILTRATR_COMMON_BUILD_SHARED OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    InfiltratrCommon
    GIT_REPOSITORY https://github.com/Infiltrator-Projects/Infiltrator-Libraries.git
    GIT_TAG a9db06b11f493c4e6f42bf6c13cc5cc5c73e1fc4
)
FetchContent_MakeAvailable(InfiltratrCommon)

find_package(X11 REQUIRED)
find_package(ZLIB REQUIRED)

add_library(backyard-gameplay STATIC
    src/gameplay.cpp
    src/race_session.cpp)
target_include_directories(backyard-gameplay PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_features(backyard-gameplay PUBLIC cxx_std_20)
set_target_properties(backyard-gameplay PROPERTIES CXX_EXTENSIONS OFF)

# Menu, Classifieds and Garage now render complete software-pixel frames and
# present the finished image through X11. The remaining screens temporarily
# retain the proven legacy scene/race path while the framebuffer migration
# continues. Prototype scene assets are embedded and decoded once at startup.
add_executable(backyard-racer src/main_framebuffer_pass.cpp)
target_compile_features(backyard-racer PRIVATE cxx_std_20)
set_target_properties(backyard-racer PROPERTIES CXX_EXTENSIONS OFF)
target_link_libraries(backyard-racer PRIVATE
    backyard-gameplay
    InfiltratrCommon::Portable
    X11::X11
    ZLIB::ZLIB)

add_executable(backyard-gameplay-test tests/gameplay_test.cpp)
target_link_libraries(backyard-gameplay-test PRIVATE backyard-gameplay)

add_executable(backyard-race-session-test tests/race_session_test.cpp)
target_link_libraries(backyard-race-session-test PRIVATE backyard-gameplay InfiltratrCommon::Portable)

enable_testing()
add_test(NAME gameplay-smoke COMMAND backyard-gameplay-test)
add_test(NAME race-session-common-timing COMMAND backyard-race-session-test)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_compile_options(backyard-gameplay PRIVATE -Wall -Wextra -Wpedantic)
    # The migrated renderer builds the visible frame in pixel memory. The
    # legacy fallback screens still use the existing X11 fast-path helpers.
    target_compile_options(backyard-racer PRIVATE
        -Wall -Wextra -Wpedantic
        -include ${CMAKE_CURRENT_SOURCE_DIR}/src/x11_fast_path.h)
    target_compile_options(backyard-gameplay-test PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(backyard-race-session-test PRIVATE -Wall -Wextra -Wpedantic)
endif()
