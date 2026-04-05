# MazarbuLib

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

**Thread safety** — MazarbuLib is single-threaded. If `mazarbulib_feed_char`
is called from a UART ISR while `mazarbulib_tick` runs in the main loop,
protect the context with a critical section appropriate to your platform.

**Float formatting** — `float` and `double` rows use `snprintf` with `%f`.
On Cortex-M0 targets with newlib-nano you may need the linker flag
`-u _printf_float` to enable floating-point printf support.

**Label length** — labels longer than `MAZARBULIB_LABEL_WIDTH` characters
are not truncated; they extend past the column border. Keep labels within
the configured width.

## License

MIT — see [LICENSE](LICENSE).
