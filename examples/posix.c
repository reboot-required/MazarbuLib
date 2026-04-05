// Copyright (c) 2026 Lukas Kraft
// https://github.com/reboot-required
//
// Part of MazarbuLib — a UART screen display library for embedded systems.
// Named after the Book of Mazarbul from J.R.R. Tolkien's writings.
//
// SPDX-License-Identifier: MIT

// POSIX / host-testing example.
//
// Renders a live-updating table to stdout. Useful for iterating on screen
// layouts and value formatting without target hardware.
//
// Build (from repo root):
//   gcc -std=c99 -Wall -Wextra -Wpedantic \
//       -Iinclude examples/posix.c src/mazarbulib.c -o mazarbulib_demo
//
// Run: ./mazarbulib_demo
//
// Note: UART navigation (n/p keys) is not wired up in this example.

#include "mazarbulib.h"

#include <stdio.h>
#include <unistd.h>

static mazarbulib_t g_lib;
static float temperature = 23.5f;
static int32_t rpm = 1200;

void uart_send(const char *data, size_t len) {
  fwrite(data, 1, len, stdout);
  fflush(stdout);
}

void terminal_clear(void) {
  static const char kSeq[] = "\033[2J\033[H";
  uart_send(kSeq, sizeof(kSeq) - 1);
}

int main(void) {
  mazarbulib_init(&g_lib, uart_send, terminal_clear);

  int s0 = mazarbulib_register_screen(&g_lib, "Engine Monitor");
  mazarbulib_register_row(&g_lib, s0, "Temperature", MAZARBULIB_TYPE_FLOAT,
                          &temperature);
  mazarbulib_register_row(&g_lib, s0, "RPM", MAZARBULIB_TYPE_INT32, &rpm);

  for (;;) {
    mazarbulib_tick(&g_lib);
    sleep(1);
  }

  return 0;
}
