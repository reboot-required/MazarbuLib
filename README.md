# MazarbuLib

[![CI](https://github.com/reboot-required/MazarbuLib/actions/workflows/ci.yml/badge.svg)](https://github.com/reboot-required/MazarbuLib/actions/workflows/ci.yml)

A portable, static-allocation C library for displaying tabular screens of data
over UART. Designed to be embedded in projects as a git submodule — no dynamic
allocation, no OS dependencies, one translation unit.

## Name

*MazarbuLib* is named after the **Book of Mazarbul** — the dwarf record-book
discovered by the Fellowship in the Chamber of Mazarbul in Moria, as described
in J.R.R. Tolkien's *The Lord of the Rings*. Like those carved records, this
library keeps structured data visible and readable.

## Features

- Platform-agnostic: user provides a `uart_send` function pointer
- Multiple named screens, each with an independent row table
- Supported value types: `int32_t`, `uint32_t`, `float`, `double`,
  `const char *`, `bool`, hex (`uint32_t` as `0xXXXXXXXX`)
- UART navigation: feed received bytes to switch screens
- Static allocation only — no `malloc`, all limits set at compile time
- C99, zero external dependencies

## Quick Start

### 1. Add as git submodule

```bash
git submodule add https://github.com/reboot-required/MazarbuLib extern/mazarbulib
```

### 2. Integrate — CMake

```cmake
add_subdirectory(extern/mazarbulib)
target_link_libraries(my_target PRIVATE mazarbulib)
```

### 3. Integrate — plain Makefile

Add `extern/mazarbulib/include` to your include path and compile
`extern/mazarbulib/src/mazarbulib.c` alongside your sources.

### 4. Use in code

```c
#include "mazarbulib.h"
#include <string.h>

static mazarbulib_t g_lib;
static float        temperature = 23.5f;
static int32_t      rpm = 1200;

// Your platform UART transmit function.
void uart_write(const char *data, size_t len) {
  // e.g. HAL_UART_Transmit(&huart1, (uint8_t *)data, len, HAL_MAX_DELAY);
}

// Optional: clear terminal with ANSI escape codes.
void terminal_clear(void) {
  static const char kSeq[] = "\033[2J\033[H";
  uart_write(kSeq, sizeof(kSeq) - 1);
}

void app_init(void) {
  mazarbulib_init(&g_lib, uart_write, terminal_clear);

  int s0 = mazarbulib_register_screen(&g_lib, "Engine Monitor");
  mazarbulib_register_row(&g_lib, s0, "Temperature",
                          MAZARBULIB_TYPE_FLOAT, &temperature);
  mazarbulib_register_row(&g_lib, s0, "RPM",
                          MAZARBULIB_TYPE_INT32, &rpm);
}

// Call at desired refresh rate (e.g. from a 250 ms timer).
void app_tick(void) {
  mazarbulib_tick(&g_lib);
}

// Call from UART RX interrupt / callback.
void uart_rx_callback(char c) {
  mazarbulib_feed_char(&g_lib, c);
}
```

### Output example

```
=== Engine Monitor ===
+----------------------+-----------------+
| Temperature          |           23.50 |
| RPM                  |            1200 |
+----------------------+-----------------+
[n]ext  [p]rev  (1/2)
```

## Building

### CMake (recommended)

```bash
cmake -B build
cmake --build build
```

Optional flags:

| Flag | Default | Effect |
|------|---------|--------|
| `-DMAZARBULIB_BUILD_TESTS=ON` | `OFF` | Build and register the test binary |
| `-DMAZARBULIB_BUILD_EXAMPLES=ON` | `OFF` | Build the POSIX demo executable |

Run tests after configuring with `-DMAZARBULIB_BUILD_TESTS=ON`:

```bash
ctest --test-dir build --output-on-failure
```

### Plain Makefile

```bash
make                 # builds libmazarbulib.a
make posix-example   # builds mazarbulib_posix_demo
make test            # builds and runs the test suite
make clean
```

### Minimum supported toolchain

| Toolchain | Minimum version | Notes |
|-----------|-----------------|-------|
| GCC       | 5               | `-std=c99 -Wall -Wextra -Wpedantic` |
| Clang     | 3.5             | same flags |
| CMake     | 3.13            | required for `target_compile_features` |
| C standard | C99            | no C11 or compiler extensions |

## Platform Integration

Your UART send function and `terminal_clear` are the only platform-specific
code you need to write. Ready-to-use starting points are in
[`examples/`](examples/):

| Target               | File                                              |
|----------------------|---------------------------------------------------|
| STM32 (HAL)          | [`examples/stm32_hal.c`](examples/stm32_hal.c)   |
| Arduino              | [`examples/arduino.cpp`](examples/arduino.cpp)   |
| POSIX / host testing | [`examples/posix.c`](examples/posix.c)           |

Pass `NULL` for `terminal_clear` on targets without ANSI terminal support;
the library will render without clearing first, causing the output to scroll
instead of refreshing in place.

## Configuration

All limits are set at compile time via `#define`. Override them in your build
system (e.g. `-DMAZARBULIB_LABEL_WIDTH=24`) **before** `mazarbulib_config.h`
is included.

| Define                          | Default | Description                  |
|---------------------------------|---------|------------------------------|
| `MAZARBULIB_MAX_SCREENS`        | `8`     | Maximum screens              |
| `MAZARBULIB_MAX_ROWS_PER_SCREEN`| `16`    | Maximum rows per screen      |
| `MAZARBULIB_LABEL_WIDTH`        | `20`    | Label column width (chars)   |
| `MAZARBULIB_VALUE_WIDTH`        | `15`    | Value column width (chars)   |
| `MAZARBULIB_NAV_NEXT`           | `'n'`   | Next-screen UART character   |
| `MAZARBULIB_NAV_PREV`           | `'p'`   | Previous-screen UART char    |

## Notes

**`MAZARBULIB_TYPE_STRING` contract** — `value_ptr` must be a non-NULL
`const char *` pointing directly to the string data. Pass the pointer itself,
not its address:

```c
char status[16] = "idle";
mazarbulib_register_row(&lib, s, "Status", MAZARBULIB_TYPE_STRING, status);
// Later:
strncpy(status, "running", sizeof(status) - 1);
status[sizeof(status) - 1] = '\0'; // next mazarbulib_tick() will show "running"
```

For an empty string pass `""`. To display a string whose pointer may change
at runtime, keep the pointer itself stable (e.g. a fixed-size char array or a
persistent buffer) and update its contents in place.

**Truncation** — labels longer than `MAZARBULIB_LABEL_WIDTH` and values
longer than `MAZARBULIB_VALUE_WIDTH` are silently truncated so that table
borders remain aligned. Screen names longer than
`MAZARBULIB_LABEL_WIDTH + MAZARBULIB_VALUE_WIDTH + 4` are likewise truncated
in the title line.

**Thread safety** — MazarbuLib is single-threaded. If `mazarbulib_feed_char`
is called from a UART ISR while `mazarbulib_tick` runs in the main loop,
protect the context with a critical section appropriate to your platform.

**Float formatting** — `float` and `double` rows use `snprintf` with `%f`.
On Cortex-M0 targets with newlib-nano you may need the linker flag
`-u _printf_float` to enable floating-point printf support.

**Config limits** — `MAZARBULIB_MAX_SCREENS` and `MAZARBULIB_MAX_ROWS_PER_SCREEN`
are stored in `uint8_t` counters. Values above 255 produce a compile-time error.

## License

MIT — see [LICENSE](LICENSE).
