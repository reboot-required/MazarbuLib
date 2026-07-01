// Copyright (c) 2026 Lukas Kraft
// https://github.com/reboot-required
//
// Part of MazarbuLib, a UART screen display library for embedded systems.
// Named after the Book of Mazarbul from J.R.R. Tolkien's writings.
//
// SPDX-License-Identifier: MIT

// POSIX / host-testing example. Reference integration, not production-ready.
//
// Renders a live-updating table to stdout, useful for iterating on screen
// layouts and value formatting without target hardware. UART navigation
// (n/p keys) is not wired up here.
//
// Build (from repo root):
//   make posix-example
// or directly:
//   gcc -std=c99 -Wall -Wextra -Wpedantic -Iinclude \
//       examples/posix.c src/mazarbulib.c -o mazarbulib_posix_demo
//
// Run: ./mazarbulib_posix_demo

#include "mazarbulib.h"

#include <stdio.h>
#include <unistd.h>

static mazarbulib_t g_lib;
static float temperature = 23.5f;
static int32_t rpm = 1200;

static void uart_send(const char *data, size_t len) {
  fwrite(data, 1, len, stdout);
  fflush(stdout);
}

static void terminal_clear(void) {
  static const char kSeq[] = "\033[2J\033[H";
  uart_send(kSeq, sizeof(kSeq) - 1);
}

int main(void) {
  if (mazarbulib_init(&g_lib, uart_send, terminal_clear) != MAZARBULIB_ERR_OK) {
    fprintf(stderr, "mazarbulib_init failed\n");
    return 1;
  }

  int s0 = mazarbulib_register_screen(&g_lib, "Engine Monitor");
  if (s0 < 0) {
    fprintf(stderr, "mazarbulib_register_screen failed: %d\n", s0);
    return 1;
  }
  if (mazarbulib_register_row(&g_lib, s0, "Temperature", MAZARBULIB_TYPE_FLOAT,
                              &temperature) != MAZARBULIB_ERR_OK ||
      mazarbulib_register_row(&g_lib, s0, "RPM", MAZARBULIB_TYPE_INT32, &rpm) !=
          MAZARBULIB_ERR_OK) {
    fprintf(stderr, "mazarbulib_register_row failed\n");
    return 1;
  }

  for (;;) {
    mazarbulib_tick(&g_lib);
    sleep(1);
  }
}
