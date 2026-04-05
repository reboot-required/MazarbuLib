// Copyright (c) 2026 Lukas Kraft
// https://github.com/reboot-required
//
// Part of MazarbuLib — a UART screen display library for embedded systems.
// Named after the Book of Mazarbul from J.R.R. Tolkien's writings.
//
// SPDX-License-Identifier: MIT

#include "mazarbulib.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

// Internal line buffer large enough for any formatted table line.
// Row layout: "| " + label + " | " + value + " |\r\n\0"
#define MAZARBULIB_LINE_BUF_SIZE_                                              \
  (MAZARBULIB_LABEL_WIDTH + MAZARBULIB_VALUE_WIDTH + 16)

// Dash count for each table border segment (column width + two spaces).
#define MAZARBULIB_LABEL_SEG_LEN_ (MAZARBULIB_LABEL_WIDTH + 2)
#define MAZARBULIB_VALUE_SEG_LEN_ (MAZARBULIB_VALUE_WIDTH + 2)

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

// Transmits the result of an snprintf call, clamping to the bytes actually
// written into buf (handles the case where n >= buf_size due to truncation).
static void mazarbulib_send_line_(mazarbulib_t *ctx, const char *buf, int n,
                                  size_t buf_size) {
  if (n <= 0) {
    return;
  }
  ctx->uart_send(buf, ((size_t)n < buf_size) ? (size_t)n : buf_size - 1);
}

// Writes the "+---...---+---...---+\r\n" border directly to UART.
static void mazarbulib_send_separator_(mazarbulib_t *ctx) {
  char line[MAZARBULIB_LINE_BUF_SIZE_];
  size_t pos = 0;

  line[pos++] = '+';
  memset(line + pos, '-', MAZARBULIB_LABEL_SEG_LEN_);
  pos += MAZARBULIB_LABEL_SEG_LEN_;
  line[pos++] = '+';
  memset(line + pos, '-', MAZARBULIB_VALUE_SEG_LEN_);
  pos += MAZARBULIB_VALUE_SEG_LEN_;
  line[pos++] = '+';
  line[pos++] = '\r';
  line[pos++] = '\n';

  ctx->uart_send(line, pos);
}

// Returns true for value types that are right-aligned in the value column.
static bool mazarbulib_is_right_aligned_(mazarbulib_type_t type) {
  return type == MAZARBULIB_TYPE_INT32 || type == MAZARBULIB_TYPE_UINT32 ||
         type == MAZARBULIB_TYPE_FLOAT || type == MAZARBULIB_TYPE_DOUBLE ||
         type == MAZARBULIB_TYPE_HEX;
}

// Formats row->value_ptr into buf according to its type.
// buf_len must be at least MAZARBULIB_VALUE_WIDTH + 1.
static void mazarbulib_format_value_(const mazarbulib_row_t *row, char *buf,
                                     size_t buf_len) {
  switch (row->type) {
  case MAZARBULIB_TYPE_INT32:
    snprintf(buf, buf_len, "%" PRId32, *(const int32_t *)row->value_ptr);
    break;
  case MAZARBULIB_TYPE_UINT32:
    snprintf(buf, buf_len, "%" PRIu32, *(const uint32_t *)row->value_ptr);
    break;
  case MAZARBULIB_TYPE_FLOAT:
    snprintf(buf, buf_len, "%.2f", (double)*(const float *)row->value_ptr);
    break;
  case MAZARBULIB_TYPE_DOUBLE:
    snprintf(buf, buf_len, "%.2f", *(const double *)row->value_ptr);
    break;
  case MAZARBULIB_TYPE_STRING:
    snprintf(buf, buf_len, "%s", (const char *)row->value_ptr);
    break;
  case MAZARBULIB_TYPE_BOOL:
    snprintf(buf, buf_len, "%s",
             *(const bool *)row->value_ptr ? "true" : "false");
    break;
  case MAZARBULIB_TYPE_HEX:
    snprintf(buf, buf_len, "0x%08" PRIX32, *(const uint32_t *)row->value_ptr);
    break;
  default:
    snprintf(buf, buf_len, "?");
    break;
  }
}

// Renders the currently active screen to UART.
static void mazarbulib_render_screen_(mazarbulib_t *ctx) {
  char line[MAZARBULIB_LINE_BUF_SIZE_];
  const mazarbulib_screen_t *s = &ctx->screens[ctx->active_screen];
  int n;

  // Title.
  n = snprintf(line, sizeof(line), "=== %s ===\r\n", s->name);
  mazarbulib_send_line_(ctx, line, n, sizeof(line));

  mazarbulib_send_separator_(ctx);

  for (uint8_t i = 0; i < s->row_count; i++) {
    const mazarbulib_row_t *row = &s->rows[i];
    char val_buf[MAZARBULIB_VALUE_WIDTH + 1];

    mazarbulib_format_value_(row, val_buf, sizeof(val_buf));

    if (mazarbulib_is_right_aligned_(row->type)) {
      n = snprintf(line, sizeof(line), "| %-*s | %*s |\r\n",
                   MAZARBULIB_LABEL_WIDTH, row->label, MAZARBULIB_VALUE_WIDTH,
                   val_buf);
    } else {
      n = snprintf(line, sizeof(line), "| %-*s | %-*s |\r\n",
                   MAZARBULIB_LABEL_WIDTH, row->label, MAZARBULIB_VALUE_WIDTH,
                   val_buf);
    }
    mazarbulib_send_line_(ctx, line, n, sizeof(line));
  }

  mazarbulib_send_separator_(ctx);

  // Navigation footer.
  n = snprintf(line, sizeof(line), "[%c]ext  [%c]rev  (%u/%u)\r\n",
               MAZARBULIB_NAV_NEXT, MAZARBULIB_NAV_PREV,
               (unsigned)(ctx->active_screen + 1u),
               (unsigned)ctx->screen_count);
  mazarbulib_send_line_(ctx, line, n, sizeof(line));
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

mazarbulib_err_t mazarbulib_init(mazarbulib_t *ctx,
                                 void (*uart_send)(const char *data,
                                                   size_t len),
                                 void (*screen_clear)(void)) {
  if (ctx == NULL || uart_send == NULL) {
    return MAZARBULIB_ERR_INVALID;
  }
  memset(ctx, 0, sizeof(*ctx));
  ctx->uart_send = uart_send;
  ctx->screen_clear = screen_clear;
  return MAZARBULIB_ERR_OK;
}

int mazarbulib_register_screen(mazarbulib_t *ctx, const char *name) {
  if (ctx == NULL || name == NULL) {
    return (int)MAZARBULIB_ERR_INVALID;
  }
  if (ctx->screen_count >= MAZARBULIB_MAX_SCREENS) {
    return (int)MAZARBULIB_ERR_FULL;
  }
  int idx = (int)ctx->screen_count;
  ctx->screens[idx].name = name;
  ctx->screens[idx].row_count = 0;
  ctx->screen_count++;
  return idx;
}

mazarbulib_err_t mazarbulib_register_row(mazarbulib_t *ctx, int screen_idx,
                                         const char *label,
                                         mazarbulib_type_t type,
                                         const void *value_ptr) {
  if (ctx == NULL || label == NULL || value_ptr == NULL) {
    return MAZARBULIB_ERR_INVALID;
  }
  if (screen_idx < 0 || screen_idx >= (int)ctx->screen_count) {
    return MAZARBULIB_ERR_INVALID;
  }
  mazarbulib_screen_t *s = &ctx->screens[(uint8_t)screen_idx];
  if (s->row_count >= MAZARBULIB_MAX_ROWS_PER_SCREEN) {
    return MAZARBULIB_ERR_FULL;
  }
  mazarbulib_row_t *row = &s->rows[s->row_count];
  row->label = label;
  row->type = type;
  row->value_ptr = value_ptr;
  s->row_count++;
  return MAZARBULIB_ERR_OK;
}

void mazarbulib_next_screen(mazarbulib_t *ctx) {
  if (ctx == NULL || ctx->screen_count == 0) {
    return;
  }
  ctx->active_screen = (uint8_t)((ctx->active_screen + 1u) % ctx->screen_count);
}

void mazarbulib_prev_screen(mazarbulib_t *ctx) {
  if (ctx == NULL || ctx->screen_count == 0) {
    return;
  }
  ctx->active_screen = (ctx->active_screen == 0) ? ctx->screen_count - 1u
                                                 : ctx->active_screen - 1u;
}

void mazarbulib_set_screen(mazarbulib_t *ctx, uint8_t screen_idx) {
  if (ctx == NULL || screen_idx >= ctx->screen_count) {
    return;
  }
  ctx->active_screen = screen_idx;
}

void mazarbulib_feed_char(mazarbulib_t *ctx, char c) {
  if (ctx == NULL) {
    return;
  }
  if (c == MAZARBULIB_NAV_NEXT) {
    mazarbulib_next_screen(ctx);
  } else if (c == MAZARBULIB_NAV_PREV) {
    mazarbulib_prev_screen(ctx);
  }
}

void mazarbulib_tick(mazarbulib_t *ctx) {
  if (ctx == NULL || ctx->screen_count == 0) {
    return;
  }
  if (ctx->screen_clear != NULL) {
    ctx->screen_clear();
  }
  mazarbulib_render_screen_(ctx);
}
