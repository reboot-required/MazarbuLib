# Integration Guide

MazarbuLib needs two things from your platform: a way to send bytes over UART,
and optionally a way to clear the terminal. Everything else is portable C99.
This page covers those callbacks, how to drive the library, and the target
specifics worth knowing.

## Adding the library

As a git submodule, with CMake:

```cmake
add_subdirectory(extern/mazarbulib)
target_link_libraries(my_target PRIVATE mazarbulib)
```

Or compile `src/mazarbulib.c` directly and add `include/` to your include path.
See the [Tooling Guide](tooling.md) for the full build matrix.

## The two callbacks

### uart_send (required)

```c
void uart_send(const char *data, size_t len);
```

Transmit exactly `len` bytes starting at `data`. The library calls this once per
rendered line. It never sends a trailing NUL, so do not treat `data` as a
C string; honour `len`.

Blocking transmit is fine for a periodic refresh. If you use DMA or an
interrupt-driven queue, make sure the bytes are copied or held until the
transfer completes, because the source buffer is a stack buffer that is reused
on the next line.

### screen_clear (optional)

```c
void screen_clear(void);
```

Called before each redraw. On an ANSI terminal, emit the clear-and-home
sequence:

```c
void screen_clear(void) {
  static const char seq[] = "\033[2J\033[H";
  uart_send(seq, sizeof(seq) - 1);
}
```

Pass `NULL` for `screen_clear` on targets without ANSI support. The library then
renders without clearing, so frames scroll instead of refreshing in place.

## Driving the library

Register screens and rows once at startup, then tick periodically and feed
received bytes:

```c
static mazarbulib_t g_lib;
static int32_t rpm = 0;

void app_init(void) {
  mazarbulib_init(&g_lib, uart_send, screen_clear);
  int s = mazarbulib_register_screen(&g_lib, "Engine Monitor");
  mazarbulib_register_row(&g_lib, s, "RPM", MAZARBULIB_TYPE_INT32, &rpm);
}

void app_tick(void)          { mazarbulib_tick(&g_lib); }   // e.g. every 250 ms
void uart_rx(char c)         { mazarbulib_feed_char(&g_lib, c); }  // from RX
```

Because rows hold a pointer to your value, updating `rpm` anywhere is enough;
the next tick shows the new number. Nothing is copied.

## Reference examples

Ready-to-adapt starting points live in [`examples/`](../examples):

| Target | File |
|--------|------|
| STM32 (HAL) | [`examples/stm32_hal.c`](../examples/stm32_hal.c) |
| Arduino | [`examples/arduino.cpp`](../examples/arduino.cpp) |
| POSIX / host | [`examples/posix.c`](../examples/posix.c) |

These are reference integrations, not production code. Adapt the peripheral
setup and error handling to your project.

## Target specifics

### ISR safety

`mazarbulib_feed_char` is typically called from the UART RX interrupt while
`mazarbulib_tick` runs in the main loop. Both mutate the same context. If your
platform can interleave them, wrap the shared access in a critical section. The
library takes no locks itself.

### Refresh rate

`mazarbulib_tick` emits a full frame every call. Pick a rate that balances
readability against UART bandwidth: at 115200 baud a frame of a few hundred
bytes fits comfortably several times a second. A 250 ms timer is a reasonable
default.

### Float printf on newlib-nano

Float and double rows format with `snprintf` and `%.2f`. On Cortex-M0 targets
using newlib-nano, floating-point `printf` is off by default; add the linker
flag `-u _printf_float` to enable it, or avoid the float and double row types.

### Compile-time limits

The screen and row counts are stored in `uint8_t`. Raising
`MAZARBULIB_MAX_SCREENS` or `MAZARBULIB_MAX_ROWS_PER_SCREEN` above 255 triggers a
compile-time error by design; see the
[design decisions](design-decisions.md#compile-time-counter-limits).

### The string row contract

`MAZARBULIB_TYPE_STRING` expects `value_ptr` to be a non-NULL, NUL-terminated
`const char *` pointing directly at the string data. Pass the buffer, not its
address. For an empty string pass `""`. To display text that changes at runtime,
keep the pointer stable (a fixed char array or persistent buffer) and update its
contents in place:

```c
char status[16] = "idle";
mazarbulib_register_row(&lib, s, "Status", MAZARBULIB_TYPE_STRING, status);
// Later:
strncpy(status, "running", sizeof(status) - 1);
status[sizeof(status) - 1] = '\0';   // next tick shows "running"
```

Labels and values longer than their column widths are truncated to keep borders
aligned; see the [design decisions](design-decisions.md#truncation-over-error).
