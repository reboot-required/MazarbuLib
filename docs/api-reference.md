# API Reference

The public API is declared in [`include/mazarbulib.h`](../include/mazarbulib.h),
with tunable limits in
[`include/mazarbulib_config.h`](../include/mazarbulib_config.h). The header
carries doc comments on every declaration; this page collects the same
information with return codes and worked examples in one place.

All functions are safe to call with a `NULL` context: fallible functions return
`MAZARBULIB_ERR_INVALID`, and void functions are no-ops.

## Lifecycle

### mazarbulib_init

```c
mazarbulib_err_t mazarbulib_init(mazarbulib_t *ctx,
                                 void (*uart_send)(const char *data, size_t len),
                                 void (*screen_clear)(void));
```

Zeroes the context and stores the callbacks. Call once before any other
function.

| Parameter | Notes |
|-----------|-------|
| `ctx` | Non-NULL pointer to a statically allocated `mazarbulib_t`. |
| `uart_send` | Non-NULL. Transmits `len` bytes from `data` over UART. |
| `screen_clear` | Called before every redraw, or `NULL` to skip clearing. |

Returns `MAZARBULIB_ERR_OK`, or `MAZARBULIB_ERR_INVALID` if `ctx` or
`uart_send` is `NULL`.

## Registration

### mazarbulib_register_screen

```c
int mazarbulib_register_screen(mazarbulib_t *ctx, const char *name);
```

Registers a named screen and returns its index for use with
`mazarbulib_register_row`. The `name` pointer is stored, not copied, and must
outlive the context.

Returns the screen index (`>= 0`) on success, `MAZARBULIB_ERR_FULL` (`-1`) when
`MAZARBULIB_MAX_SCREENS` is exhausted, or `MAZARBULIB_ERR_INVALID` (`-2`) when
`ctx` or `name` is `NULL`. Test the result with `< 0` for errors.

### mazarbulib_register_row

```c
mazarbulib_err_t mazarbulib_register_row(mazarbulib_t *ctx, int screen_idx,
                                         const char *label,
                                         mazarbulib_type_t type,
                                         const void *value_ptr);
```

Adds a data row to the screen at `screen_idx`. Both `label` and `value_ptr` are
stored by reference and must outlive the context. `value_ptr` is dereferenced at
render time, so the current value is always shown.

Pass the address of the variable for numeric and bool types; pass the pointer
itself for strings:

```c
int32_t rpm = 0;
mazarbulib_register_row(&lib, s, "RPM", MAZARBULIB_TYPE_INT32, &rpm);

char state[16] = "idle";
mazarbulib_register_row(&lib, s, "State", MAZARBULIB_TYPE_STRING, state);
```

Returns `MAZARBULIB_ERR_OK`, `MAZARBULIB_ERR_FULL` when the screen's row table
is full, or `MAZARBULIB_ERR_INVALID` when `ctx`, `label`, or `value_ptr` is
`NULL`, or `screen_idx` is out of range.

## Navigation

None of these produce output; they change `active_screen`, and the next
`mazarbulib_tick` reflects it. All are no-ops when `ctx` is `NULL` or no screens
are registered.

### mazarbulib_next_screen

```c
void mazarbulib_next_screen(mazarbulib_t *ctx);
```

Advances to the next screen, wrapping from the last back to the first.

### mazarbulib_prev_screen

```c
void mazarbulib_prev_screen(mazarbulib_t *ctx);
```

Goes back one screen, wrapping from the first to the last.

### mazarbulib_set_screen

```c
void mazarbulib_set_screen(mazarbulib_t *ctx, uint8_t screen_idx);
```

Jumps to `screen_idx`. No-op if `screen_idx` is out of range.

### mazarbulib_feed_char

```c
void mazarbulib_feed_char(mazarbulib_t *ctx, char c);
```

Feeds one received UART byte. A byte equal to `MAZARBULIB_NAV_NEXT` advances,
`MAZARBULIB_NAV_PREV` goes back, and any other byte is ignored. Call it from
your RX interrupt or polling loop.

## Rendering

### mazarbulib_tick

```c
void mazarbulib_tick(mazarbulib_t *ctx);
```

Renders the active screen over UART. Call it periodically at the refresh rate
you want (for example from a 250 ms timer). Calls `screen_clear` first when it
is non-NULL. No-op when `ctx` is `NULL` or no screens are registered.

## Types

### mazarbulib_type_t

The value type tag stored on each row.

| Value | C type | Rendered as |
|-------|--------|-------------|
| `MAZARBULIB_TYPE_INT32` | `int32_t` | signed decimal |
| `MAZARBULIB_TYPE_UINT32` | `uint32_t` | unsigned decimal |
| `MAZARBULIB_TYPE_FLOAT` | `float` | two decimal places |
| `MAZARBULIB_TYPE_DOUBLE` | `double` | two decimal places |
| `MAZARBULIB_TYPE_STRING` | `const char *` | rendered directly |
| `MAZARBULIB_TYPE_BOOL` | `bool` | `true` / `false` |
| `MAZARBULIB_TYPE_HEX` | `uint32_t` | `0xXXXXXXXX` |

### mazarbulib_err_t

| Value | Numeric | Meaning |
|-------|---------|---------|
| `MAZARBULIB_ERR_OK` | `0` | Success. |
| `MAZARBULIB_ERR_FULL` | `-1` | Screen table or row table is full. |
| `MAZARBULIB_ERR_INVALID` | `-2` | NULL pointer or out-of-range argument. |

### mazarbulib_row_t, mazarbulib_screen_t, mazarbulib_t

The context and its nested structs are described in
[Architecture](architecture.md#data-model). Declare one `mazarbulib_t`
statically and do not modify its fields directly after `mazarbulib_init`.

## Configuration macros

Override these before `mazarbulib_config.h` is included, for example with
`-DMAZARBULIB_LABEL_WIDTH=24`.

| Macro | Default | Meaning |
|-------|---------|---------|
| `MAZARBULIB_MAX_SCREENS` | `8` | Maximum screens (must be `<= 255`). |
| `MAZARBULIB_MAX_ROWS_PER_SCREEN` | `16` | Maximum rows per screen (must be `<= 255`). |
| `MAZARBULIB_LABEL_WIDTH` | `20` | Label column width in characters. |
| `MAZARBULIB_VALUE_WIDTH` | `15` | Value column width in characters. |
| `MAZARBULIB_NAV_NEXT` | `'n'` | Byte that navigates to the next screen. |
| `MAZARBULIB_NAV_PREV` | `'p'` | Byte that navigates to the previous screen. |

## Version macros

| Macro | Meaning |
|-------|---------|
| `MAZARBULIB_VERSION_MAJOR` / `MINOR` / `PATCH` | Numeric version parts. |
| `MAZARBULIB_VERSION_STRING` | String form, for example `"0.1.2"`. |
| `MAZARBULIB_VERSION` | Encoded `0xMMNNPP` for ordered comparisons. |
| `MAZARBULIB_VERSION_ENCODE(major, minor, patch)` | Encodes parts the same way. |

```c
#if MAZARBULIB_VERSION >= MAZARBULIB_VERSION_ENCODE(0, 1, 2)
  // feature available since 0.1.2
#endif
```
