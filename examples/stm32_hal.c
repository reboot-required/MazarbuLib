// Copyright (c) 2026 Lukas Kraft
// https://github.com/reboot-required
//
// Part of MazarbuLib — a UART screen display library for embedded systems.
// Named after the Book of Mazarbul from J.R.R. Tolkien's writings.
//
// SPDX-License-Identifier: MIT

// STM32 HAL integration example.
//
// Requires the STM32 HAL headers included via your project's device header
// (e.g. stm32f4xx_hal.h). Adapt huart1 to the USART/UART peripheral
// connected to your terminal.

#include "mazarbulib.h"

static mazarbulib_t g_lib;
static float temperature = 23.5f;
static int32_t rpm = 1200;

void uart_send(const char *data, size_t len) {
  HAL_UART_Transmit(&huart1, (uint8_t *)data, (uint16_t)len, HAL_MAX_DELAY);
}

void terminal_clear(void) {
  static const char kSeq[] = "\033[2J\033[H";
  uart_send(kSeq, sizeof(kSeq) - 1);
}

void app_init(void) {
  mazarbulib_init(&g_lib, uart_send, terminal_clear);

  int s0 = mazarbulib_register_screen(&g_lib, "Engine Monitor");
  mazarbulib_register_row(&g_lib, s0, "Temperature", MAZARBULIB_TYPE_FLOAT,
                          &temperature);
  mazarbulib_register_row(&g_lib, s0, "RPM", MAZARBULIB_TYPE_INT32, &rpm);
}

// Call from your timer callback or main loop at the desired refresh rate.
void app_tick(void) { mazarbulib_tick(&g_lib); }

// Call from the UART RX interrupt / callback.
void uart_rx_callback(char c) { mazarbulib_feed_char(&g_lib, c); }
