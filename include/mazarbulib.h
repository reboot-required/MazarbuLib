// Copyright (c) 2026 Lukas Kraft
// https://github.com/reboot-required
//
// Part of MazarbuLib — a UART screen display library for embedded systems.
// Named after the Book of Mazarbul from J.R.R. Tolkien's writings.
//
// SPDX-License-Identifier: MIT

#ifndef MAZARBULIB_INCLUDE_MAZARBULIB_H_
#define MAZARBULIB_INCLUDE_MAZARBULIB_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "mazarbulib_config.h"

#ifdef __cplusplus
extern "C" {
#endif

// -----------------------------------------------------------------------------
// Types
// -----------------------------------------------------------------------------

// Supported value types for table rows.
typedef enum {
  MAZARBULIB_TYPE_INT32,  // int32_t, displayed as signed decimal.
  MAZARBULIB_TYPE_UINT32, // uint32_t, displayed as unsigned decimal.
  MAZARBULIB_TYPE_FLOAT,  // float, displayed with two decimal places.
  MAZARBULIB_TYPE_DOUBLE, // double, displayed with two decimal places.
  MAZARBULIB_TYPE_STRING, // const char *, rendered directly.
  MAZARBULIB_TYPE_BOOL,   // bool, displayed as "true" / "false".
  MAZARBULIB_TYPE_HEX,    // uint32_t, displayed as 0xXXXXXXXX.
} mazarbulib_type_t;

// Return codes used throughout the API.
typedef enum {
  MAZARBULIB_ERR_OK = 0,
  MAZARBULIB_ERR_FULL = -1,    // Screen table or row table is full.
  MAZARBULIB_ERR_INVALID = -2, // NULL pointer or out-of-range argument.
} mazarbulib_err_t;

// A single data row within a screen.
typedef struct {
  const char *label;      // Row label (pointer, not copied).
  mazarbulib_type_t type; // Value type.
  const void *value_ptr;  // Pointer dereferenced at render time.
} mazarbulib_row_t;

// A single named screen containing a fixed row table.
typedef struct {
  const char *name;                                      // Screen title.
  mazarbulib_row_t rows[MAZARBULIB_MAX_ROWS_PER_SCREEN]; // Row table.
  uint8_t row_count;                                     // Populated rows.
} mazarbulib_screen_t;

// Library context. Declare one instance statically in the application.
// Do not modify the fields directly after initialisation.
//
// Note: screen_count and active_screen are stored in uint8_t. If
// MAZARBULIB_MAX_SCREENS or MAZARBULIB_MAX_ROWS_PER_SCREEN is raised above
// 255 a compile-time error is produced in mazarbulib.c.
typedef struct {
  mazarbulib_screen_t screens[MAZARBULIB_MAX_SCREENS]; // Screen table.
  uint8_t screen_count;  // Number of registered screens.
  uint8_t active_screen; // Index of the currently displayed screen.

  // Transmits len bytes starting at data over UART. Must not be NULL.
  void (*uart_send)(const char *data, size_t len);

  // Called before every redraw to clear the terminal. May be NULL.
  void (*screen_clear)(void);
} mazarbulib_t;

// -----------------------------------------------------------------------------
// API
// -----------------------------------------------------------------------------

// Initialises the library context and sets UART callbacks.
//
//   ctx          Non-NULL pointer to a statically allocated mazarbulib_t.
//   uart_send    Non-NULL. Called to transmit rendered bytes over UART.
//   screen_clear Called before every redraw to clear the terminal, or NULL
//                to skip (e.g. targets without ANSI terminal support).
//
// Returns MAZARBULIB_ERR_INVALID if ctx or uart_send is NULL.
mazarbulib_err_t mazarbulib_init(mazarbulib_t *ctx,
                                 void (*uart_send)(const char *data,
                                                   size_t len),
                                 void (*screen_clear)(void));

// Registers a named screen and returns its index (>= 0) for use with
// mazarbulib_register_row(). The name pointer must remain valid for the
// lifetime of the context.
//
// Returns MAZARBULIB_ERR_FULL when MAZARBULIB_MAX_SCREENS is exhausted,
// or MAZARBULIB_ERR_INVALID when ctx or name is NULL.
int mazarbulib_register_screen(mazarbulib_t *ctx, const char *name);

// Registers a data row on the screen at screen_idx.
// label and value_ptr must remain valid for the lifetime of the context.
// value_ptr is read at render time, so the pointed-to value is always live.
//
// For numeric and bool types, pass the address of your variable:
//   int32_t rpm = 0;     register_row(..., MAZARBULIB_TYPE_INT32,  &rpm);
// For strings, pass the const char * directly (not its address):
//   char msg[32] = "hi"; register_row(..., MAZARBULIB_TYPE_STRING, msg);
//
// MAZARBULIB_TYPE_STRING: value_ptr must be a non-NULL, NUL-terminated
// const char * pointing directly to the string data. NULL is not allowed;
// for an empty string pass "".
// Label and value strings longer than MAZARBULIB_LABEL_WIDTH /
// MAZARBULIB_VALUE_WIDTH are truncated to keep table borders aligned.
//
// Returns MAZARBULIB_ERR_FULL, MAZARBULIB_ERR_INVALID, or MAZARBULIB_ERR_OK.
mazarbulib_err_t mazarbulib_register_row(mazarbulib_t *ctx, int screen_idx,
                                         const char *label,
                                         mazarbulib_type_t type,
                                         const void *value_ptr);

// Advances to the next screen, wrapping around to the first.
void mazarbulib_next_screen(mazarbulib_t *ctx);

// Goes back to the previous screen, wrapping around to the last.
void mazarbulib_prev_screen(mazarbulib_t *ctx);

// Jumps to the screen at screen_idx. No-op if ctx is NULL or screen_idx is
// out of range.
void mazarbulib_set_screen(mazarbulib_t *ctx, int screen_idx);

// Feeds a single received byte from the UART RX handler.
// Handles MAZARBULIB_NAV_NEXT and MAZARBULIB_NAV_PREV; ignores other bytes.
void mazarbulib_feed_char(mazarbulib_t *ctx, char c);

// Renders the active screen over UART. Call periodically from the main loop
// or a timer callback at the desired refresh rate. Calls screen_clear (if
// non-NULL) before rendering.
void mazarbulib_tick(mazarbulib_t *ctx);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // MAZARBULIB_INCLUDE_MAZARBULIB_H_
