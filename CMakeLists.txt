# Copyright (c) 2026 Lukas Kraft
# https://github.com/reboot-required
#
# Part of MazarbuLib — a UART screen display library for embedded systems.
# Named after the Book of Mazarbul from J.R.R. Tolkien's writings.
#
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.13)

project(mazarbulib
  VERSION 0.1.0
  LANGUAGES C
  DESCRIPTION "UART tabular screen display library for embedded systems"
)

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------

option(MAZARBULIB_BUILD_TESTS    "Build MazarbuLib tests"    OFF)
option(MAZARBULIB_BUILD_EXAMPLES "Build MazarbuLib examples" OFF)

# ---------------------------------------------------------------------------
# Library target
# ---------------------------------------------------------------------------

add_library(mazarbulib STATIC
  src/mazarbulib.c
)

target_include_directories(mazarbulib PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

target_compile_features(mazarbulib PUBLIC c_std_99)

target_compile_options(mazarbulib PRIVATE
  $<$<C_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
)

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------

if(MAZARBULIB_BUILD_TESTS)
  enable_testing()

  add_executable(mazarbulib_test tests/test_mazarbulib.c)
  target_link_libraries(mazarbulib_test PRIVATE mazarbulib)
  target_compile_options(mazarbulib_test PRIVATE
    $<$<C_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
  )

  add_test(NAME test_init             COMMAND mazarbulib_test test_init)
  add_test(NAME test_register_screen  COMMAND mazarbulib_test test_register_screen)
  add_test(NAME test_register_row     COMMAND mazarbulib_test test_register_row)
  add_test(NAME test_navigation       COMMAND mazarbulib_test test_navigation)
  add_test(NAME test_zero_screen_tick COMMAND mazarbulib_test test_zero_screen_tick)
  add_test(NAME test_rendering        COMMAND mazarbulib_test test_rendering)
  add_test(NAME test_string_type      COMMAND mazarbulib_test test_string_type)
  add_test(NAME test_all_types        COMMAND mazarbulib_test test_all_types)
  add_test(NAME test_empty_string_row COMMAND mazarbulib_test test_empty_string_row)
  add_test(NAME test_max_screens_rows COMMAND mazarbulib_test test_max_screens_rows)
endif()

# ---------------------------------------------------------------------------
# Examples (POSIX only — embedded examples are reference files, not built here)
# ---------------------------------------------------------------------------

if(MAZARBULIB_BUILD_EXAMPLES)
  add_executable(mazarbulib_posix_demo examples/posix.c)
  target_link_libraries(mazarbulib_posix_demo PRIVATE mazarbulib)
  target_compile_options(mazarbulib_posix_demo PRIVATE
    $<$<C_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
  )
endif()

# ---------------------------------------------------------------------------
# Install / export
# ---------------------------------------------------------------------------

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS mazarbulib
  EXPORT  mazarbulibtargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT mazarbulibtargets
  FILE      MazarbuLibTargets.cmake
  NAMESPACE MazarbuLib::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MazarbuLib
)

configure_package_config_file(
  cmake/MazarbuLibConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/MazarbuLibConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MazarbuLib
)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/MazarbuLibConfigVersion.cmake
  VERSION      ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/MazarbuLibConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/MazarbuLibConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MazarbuLib
)
