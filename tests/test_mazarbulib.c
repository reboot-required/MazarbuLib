// Copyright (c) 2026 Lukas Kraft
// https://github.com/reboot-required
//
// Part of MazarbuLib — a UART screen display library for embedded systems.
// Named after the Book of Mazarbul from J.R.R. Tolkien's writings.
//
// SPDX-License-Identifier: MIT

// Test harness for MazarbuLib. Self-contained, no external dependencies.
// Build via CMake (-DMAZARBULIB_BUILD_TESTS=ON) or Makefile (make test).

#include "mazarbulib.h"

#include <stdio.h>
#include <string.h>

// ---------------------------------------------------------------------------
// Minimal test infrastructure
// ---------------------------------------------------------------------------

static int g_tests_run = 0;
static int g_tests_failed = 0;

#define TEST_ASSERT(cond)                                                      \
  do {                                                                         \
    g_tests_run++;                                                             \
    if (!(cond)) {                                                             \
      fprintf(stderr, "FAIL  %s:%d  %s\n", __FILE__, __LINE__, #cond);         \
      g_tests_failed++;                                                        \
    }                                                                          \
  } while (0)

// ---------------------------------------------------------------------------
// Fake UART capture
// ---------------------------------------------------------------------------

static char g_uart_buf[4096];
static size_t g_uart_len = 0;

static void fake_uart_send(const char *data, size_t len) {
  if (g_uart_len + len < sizeof(g_uart_buf)) {
    memcpy(g_uart_buf + g_uart_len, data, len);
    g_uart_len += len;
    g_uart_buf[g_uart_len] = '\0';
  }
}

static void uart_reset(void) {
  g_uart_len = 0;
  g_uart_buf[0] = '\0';
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

static void test_init(void) {
  mazarbulib_t lib;

  // NULL ctx or uart_send must return INVALID.
  TEST_ASSERT(mazarbulib_init(NULL, fake_uart_send, NULL) ==
              MAZARBULIB_ERR_INVALID);
  TEST_ASSERT(mazarbulib_init(&lib, NULL, NULL) == MAZARBULIB_ERR_INVALID);

  // Valid init without screen_clear.
  TEST_ASSERT(mazarbulib_init(&lib, fake_uart_send, NULL) == MAZARBULIB_ERR_OK);
}

static void test_register_screen(void) {
  mazarbulib_t lib;
  mazarbulib_init(&lib, fake_uart_send, NULL);

  TEST_ASSERT(mazarbulib_register_screen(&lib, "Screen A") == 0);
  TEST_ASSERT(mazarbulib_register_screen(&lib, "Screen B") == 1);

  // NULL arguments.
  TEST_ASSERT(mazarbulib_register_screen(NULL, "X") == MAZARBULIB_ERR_INVALID);
  TEST_ASSERT(mazarbulib_register_screen(&lib, NULL) == MAZARBULIB_ERR_INVALID);

  // Fill to capacity then overflow.
  mazarbulib_t lib2;
  mazarbulib_init(&lib2, fake_uart_send, NULL);
  for (int i = 0; i < MAZARBULIB_MAX_SCREENS; i++) {
    TEST_ASSERT(mazarbulib_register_screen(&lib2, "S") >= 0);
  }
  TEST_ASSERT(mazarbulib_register_screen(&lib2, "overflow") ==
              MAZARBULIB_ERR_FULL);
}

static void test_register_row(void) {
  mazarbulib_t lib;
  mazarbulib_init(&lib, fake_uart_send, NULL);
  int s0 = mazarbulib_register_screen(&lib, "A");

  static int32_t val = 42;

  TEST_ASSERT(mazarbulib_register_row(&lib, s0, "Val", MAZARBULIB_TYPE_INT32,
                                      &val) == MAZARBULIB_ERR_OK);

  // NULL arguments.
  TEST_ASSERT(mazarbulib_register_row(NULL, s0, "V", MAZARBULIB_TYPE_INT32,
                                      &val) == MAZARBULIB_ERR_INVALID);
  TEST_ASSERT(mazarbulib_register_row(&lib, s0, NULL, MAZARBULIB_TYPE_INT32,
                                      &val) == MAZARBULIB_ERR_INVALID);
  TEST_ASSERT(mazarbulib_register_row(&lib, s0, "V", MAZARBULIB_TYPE_INT32,
                                      NULL) == MAZARBULIB_ERR_INVALID);

  // Out-of-range screen index.
  TEST_ASSERT(mazarbulib_register_row(&lib, -1, "V", MAZARBULIB_TYPE_INT32,
                                      &val) == MAZARBULIB_ERR_INVALID);
  TEST_ASSERT(mazarbulib_register_row(&lib, 99, "V", MAZARBULIB_TYPE_INT32,
                                      &val) == MAZARBULIB_ERR_INVALID);

  // Fill rows to capacity then overflow.
  mazarbulib_t lib2;
  mazarbulib_init(&lib2, fake_uart_send, NULL);
  int s = mazarbulib_register_screen(&lib2, "B");
  for (int i = 0; i < MAZARBULIB_MAX_ROWS_PER_SCREEN; i++) {
    TEST_ASSERT(mazarbulib_register_row(&lib2, s, "R", MAZARBULIB_TYPE_INT32,
                                        &val) == MAZARBULIB_ERR_OK);
  }
  TEST_ASSERT(mazarbulib_register_row(&lib2, s, "overflow",
                                      MAZARBULIB_TYPE_INT32,
                                      &val) == MAZARBULIB_ERR_FULL);
}

static void test_navigation(void) {
  mazarbulib_t lib;
  mazarbulib_init(&lib, fake_uart_send, NULL);
  mazarbulib_register_screen(&lib, "A");
  mazarbulib_register_screen(&lib, "B");
  mazarbulib_register_screen(&lib, "C");

  TEST_ASSERT(lib.active_screen == 0);

  mazarbulib_next_screen(&lib);
  TEST_ASSERT(lib.active_screen == 1);

  mazarbulib_next_screen(&lib);
  TEST_ASSERT(lib.active_screen == 2);

  mazarbulib_next_screen(&lib); // Wrap forward.
  TEST_ASSERT(lib.active_screen == 0);

  mazarbulib_prev_screen(&lib); // Wrap backward.
  TEST_ASSERT(lib.active_screen == 2);

  mazarbulib_set_screen(&lib, 1);
  TEST_ASSERT(lib.active_screen == 1);

  mazarbulib_set_screen(&lib, 99); // Out of range: no-op.
  TEST_ASSERT(lib.active_screen == 1);

  mazarbulib_set_screen(&lib, 0);
  mazarbulib_feed_char(&lib, MAZARBULIB_NAV_NEXT);
  TEST_ASSERT(lib.active_screen == 1);

  mazarbulib_feed_char(&lib, MAZARBULIB_NAV_PREV);
  TEST_ASSERT(lib.active_screen == 0);

  // Ignored byte must not change screen.
  mazarbulib_feed_char(&lib, 'x');
  TEST_ASSERT(lib.active_screen == 0);
}

static void test_zero_screen_tick(void) {
  mazarbulib_t lib;
  mazarbulib_init(&lib, fake_uart_send, NULL);
  uart_reset();

  // tick() on a context with no registered screens must not transmit anything.
  mazarbulib_tick(&lib);
  TEST_ASSERT(g_uart_len == 0);
}

static void test_rendering(void) {
  mazarbulib_t lib;
  mazarbulib_init(&lib, fake_uart_send, NULL);

  int s0 = mazarbulib_register_screen(&lib, "Engine Monitor");
  static int32_t rpm = 1200;
  mazarbulib_register_row(&lib, s0, "RPM", MAZARBULIB_TYPE_INT32, &rpm);

  uart_reset();
  mazarbulib_tick(&lib);

  TEST_ASSERT(strstr(g_uart_buf, "Engine Monitor") != NULL);
  TEST_ASSERT(strstr(g_uart_buf, "RPM") != NULL);
  TEST_ASSERT(strstr(g_uart_buf, "1200") != NULL);
  TEST_ASSERT(strstr(g_uart_buf, "1/1") != NULL);

  // The table border characters must be present.
  TEST_ASSERT(strstr(g_uart_buf, "+") != NULL);
  TEST_ASSERT(strstr(g_uart_buf, "|") != NULL);
}

static void test_string_type(void) {
  // MAZARBULIB_TYPE_STRING: value_ptr must be a const char ** (pointer to the
  // string pointer), consistent with all other types.  The render dereferences
  // it so that updates to the pointer are visible at each tick.
  mazarbulib_t lib;
  mazarbulib_init(&lib, fake_uart_send, NULL);

  int s0 = mazarbulib_register_screen(&lib, "Status");
  static const char *msg = "idle";
  mazarbulib_register_row(&lib, s0, "State", MAZARBULIB_TYPE_STRING, &msg);

  uart_reset();
  mazarbulib_tick(&lib);
  TEST_ASSERT(strstr(g_uart_buf, "idle") != NULL);

  // Update the pointer — the next render must pick up the new value.
  msg = "running";
  uart_reset();
  mazarbulib_tick(&lib);
  TEST_ASSERT(strstr(g_uart_buf, "running") != NULL);

  // NULL inner pointer must not crash; library renders empty string.
  msg = NULL;
  uart_reset();
  mazarbulib_tick(&lib);
  // As long as tick() returns without aborting the test passes.
  TEST_ASSERT(1);
}

static void test_all_types(void) {
  mazarbulib_t lib;
  mazarbulib_init(&lib, fake_uart_send, NULL);

  int s0 = mazarbulib_register_screen(&lib, "Types");

  static int32_t i32 = -42;
  static uint32_t u32 = 100u;
  static float f = 3.14f;
  static double d = 2.718;
  static const char *str = "hello";
  static bool b = true;
  static uint32_t hex = 0xDEADBEEFu;

  mazarbulib_register_row(&lib, s0, "i32", MAZARBULIB_TYPE_INT32, &i32);
  mazarbulib_register_row(&lib, s0, "u32", MAZARBULIB_TYPE_UINT32, &u32);
  mazarbulib_register_row(&lib, s0, "f", MAZARBULIB_TYPE_FLOAT, &f);
  mazarbulib_register_row(&lib, s0, "d", MAZARBULIB_TYPE_DOUBLE, &d);
  mazarbulib_register_row(&lib, s0, "str", MAZARBULIB_TYPE_STRING, &str);
  mazarbulib_register_row(&lib, s0, "bool", MAZARBULIB_TYPE_BOOL, &b);
  mazarbulib_register_row(&lib, s0, "hex", MAZARBULIB_TYPE_HEX, &hex);

  uart_reset();
  mazarbulib_tick(&lib);

  TEST_ASSERT(strstr(g_uart_buf, "-42") != NULL);
  TEST_ASSERT(strstr(g_uart_buf, "100") != NULL);
  TEST_ASSERT(strstr(g_uart_buf, "3.14") != NULL);
  TEST_ASSERT(strstr(g_uart_buf, "2.72") != NULL);
  TEST_ASSERT(strstr(g_uart_buf, "hello") != NULL);
  TEST_ASSERT(strstr(g_uart_buf, "true") != NULL);
  TEST_ASSERT(strstr(g_uart_buf, "0xDEADBEEF") != NULL);
}

static void test_empty_string_row(void) {
  mazarbulib_t lib;
  mazarbulib_init(&lib, fake_uart_send, NULL);

  int s0 = mazarbulib_register_screen(&lib, "S");
  static const char *empty = "";
  mazarbulib_register_row(&lib, s0, "label", MAZARBULIB_TYPE_STRING, &empty);

  uart_reset();
  mazarbulib_tick(&lib); // Must not crash or produce corrupt output.
  TEST_ASSERT(strstr(g_uart_buf, "label") != NULL);
}

static void test_max_screens_rows(void) {
  mazarbulib_t lib;
  mazarbulib_init(&lib, fake_uart_send, NULL);

  int last_screen = -1;
  for (int i = 0; i < MAZARBULIB_MAX_SCREENS; i++) {
    last_screen = mazarbulib_register_screen(&lib, "S");
    TEST_ASSERT(last_screen >= 0);
  }
  TEST_ASSERT(mazarbulib_register_screen(&lib, "overflow") ==
              MAZARBULIB_ERR_FULL);

  // Fill rows on the last registered screen.
  static int32_t v = 0;
  for (int i = 0; i < MAZARBULIB_MAX_ROWS_PER_SCREEN; i++) {
    TEST_ASSERT(mazarbulib_register_row(&lib, last_screen, "R",
                                        MAZARBULIB_TYPE_INT32,
                                        &v) == MAZARBULIB_ERR_OK);
  }
  TEST_ASSERT(mazarbulib_register_row(&lib, last_screen, "overflow",
                                      MAZARBULIB_TYPE_INT32,
                                      &v) == MAZARBULIB_ERR_FULL);
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

// Table of all test cases. Add new entries here when adding test functions.
typedef struct {
  const char *name;
  void (*fn)(void);
} mazarbulib_test_entry_t;

static const mazarbulib_test_entry_t k_tests[] = {
  { "test_init",              test_init              },
  { "test_register_screen",   test_register_screen   },
  { "test_register_row",      test_register_row      },
  { "test_navigation",        test_navigation        },
  { "test_zero_screen_tick",  test_zero_screen_tick  },
  { "test_rendering",         test_rendering         },
  { "test_string_type",       test_string_type       },
  { "test_all_types",         test_all_types         },
  { "test_empty_string_row",  test_empty_string_row  },
  { "test_max_screens_rows",  test_max_screens_rows  },
};

#define K_TEST_COUNT ((int)(sizeof(k_tests) / sizeof(k_tests[0])))

// Usage:
//   mazarbulib_test              - run all tests
//   mazarbulib_test <test_name>  - run one test (used by CTest per-test mode)
int main(int argc, char **argv) {
  if (argc == 2) {
    for (int i = 0; i < K_TEST_COUNT; i++) {
      if (strcmp(argv[1], k_tests[i].name) == 0) {
        k_tests[i].fn();
        if (g_tests_failed > 0) {
          fprintf(stderr, "%d/%d assertions FAILED in %s\n",
                  g_tests_failed, g_tests_run, argv[1]);
          return 1;
        }
        printf("%d/%d assertions passed in %s\n",
               g_tests_run, g_tests_run, argv[1]);
        return 0;
      }
    }
    fprintf(stderr, "Unknown test: %s\n", argv[1]);
    return 1;
  }

  for (int i = 0; i < K_TEST_COUNT; i++) {
    k_tests[i].fn();
  }

  if (g_tests_failed > 0) {
    fprintf(stderr, "%d/%d assertions FAILED\n", g_tests_failed, g_tests_run);
    return 1;
  }
  printf("%d/%d assertions passed\n", g_tests_run, g_tests_run);
  return 0;
}
