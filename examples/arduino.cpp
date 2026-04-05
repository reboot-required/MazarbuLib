// Copyright (c) 2026 Lukas Kraft
// https://github.com/reboot-required
//
// Part of MazarbuLib — a UART screen display library for embedded systems.
// Named after the Book of Mazarbul from J.R.R. Tolkien's writings.
//
// SPDX-License-Identifier: MIT

// Arduino integration example.
//
// Uses the default Serial port at 115200 baud. Swap Serial for Serial1,
// Serial2, etc. if your display terminal is on a different hardware UART.

#include "mazarbulib.h"

#include <Arduino.h>

static mazarbulib_t g_lib;
static float temperature = 23.5f;
static int32_t rpm = 1200;

extern "C" void uart_send(const char *data, size_t len) {
  Serial.write(reinterpret_cast<const uint8_t *>(data), len);
}

extern "C" void terminal_clear(void) {
  static const char kSeq[] = "\033[2J\033[H";
  uart_send(kSeq, sizeof(kSeq) - 1);
}

void setup(void) {
  Serial.begin(115200);
  mazarbulib_init(&g_lib, uart_send, terminal_clear);

  int s0 = mazarbulib_register_screen(&g_lib, "Engine Monitor");
  mazarbulib_register_row(&g_lib, s0, "Temperature", MAZARBULIB_TYPE_FLOAT,
                          &temperature);
  mazarbulib_register_row(&g_lib, s0, "RPM", MAZARBULIB_TYPE_INT32, &rpm);
}

void loop(void) {
  while (Serial.available()) {
    mazarbulib_feed_char(&g_lib, (char)Serial.read());
  }
  mazarbulib_tick(&g_lib);
  delay(250);
}
