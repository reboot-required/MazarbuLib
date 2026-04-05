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
