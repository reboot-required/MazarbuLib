# Design Decisions

Why the library is shaped the way it is.

## Static allocation

No `malloc` and friends. Everything lives in the caller's `mazarbulib_t`, sized
at compile time from `mazarbulib_config.h`, and each line is formatted in a fixed
stack buffer. Most targets are microcontrollers with no heap, and even where
there is one, a display library has no business allocating at runtime.

The catch: capacities are fixed at build time and the context reserves the full
arrays up front, so larger limits cost RAM. In exchange there is nothing to
leak, fragment, or fail.

## Compile-time counter limits

The counters (`screen_count`, `active_screen`, `row_count`) are `uint8_t`: the
counts are tiny, and one byte keeps loop indices from mixing signed and
unsigned. The price is a ceiling of 255, so the limits are guarded. Raising
`MAZARBULIB_MAX_SCREENS` or `MAZARBULIB_MAX_ROWS_PER_SCREEN` past 255 fails the
build (see [C99 compile-time asserts](#c99-compile-time-asserts)) instead of
wrapping a count at runtime. Needing more than 255 is out of scope.

## Duplicated format literals

Rows render with `snprintf`. Numbers and hex are right-aligned, strings and
bools left-aligned, and the only difference is `%*.*s` versus `%-*.*s`. Choosing
the format at runtime would trip `-Wformat-nonliteral` and switch off the
argument checking we build with under `-Wall -Wextra -Werror`.

So the two cases are two `snprintf` calls with literal formats, picked by an
`if`. Two nearly identical lines, but both stay checked, and a comment at the
call site says why so nobody folds them back together.

## Truncation over error

Labels, values, and screen names can run longer than their columns, and the
whole point of the table is that the borders line up. Oversized content is
clamped to the column width and the rest is dropped, rather than rejected or
wrapped: rejecting would make every caller handle an error for a cosmetic
problem, and wrapping would break the alignment.

The widths and `MAZARBULIB_TITLE_MAX_LEN_` bound every field, so a line cannot
overflow the buffer. If you need the whole value, widen the column with
`MAZARBULIB_LABEL_WIDTH` or `MAZARBULIB_VALUE_WIDTH`.

## C99 compile-time asserts

The 255 ceiling above wants a compile-time check, but `_Static_assert` is C11
and this library is C99. The old trick is a typedef of an array that has `1`
element when the check passes and `-1` when it fails:

```c
typedef char mazarbulib_assert_screens_fit_uint8_
    [(MAZARBULIB_MAX_SCREENS <= 255u) ? 1 : -1];
```

A negative array size will not compile, so an over-large limit stops the build
right at the guard. The typedefs are at file scope, so they draw no
unused-typedef warning, and clang-tidy is told to leave them alone. On a move to
C11 they become `_Static_assert` with a real message.

## Pull model for values

A row keeps a `const void *` to your variable plus a type tag, and reads it at
render time. To change what is shown you write the variable; the next tick picks
it up. No setters, no copies. A push model would mean typed setters, somewhere
to stash the copy, and remembering to call the setter on every change.

In return you take on a contract: keep the storage alive and stable for as long
as the context uses it, and for strings edit a fixed buffer in place instead of
reassigning the pointer. The tag has to match the real type, same as `printf`.
The read happens on the ticking thread, so a value written from an ISR needs the
same guarding as navigation (see
[function-pointer UART abstraction](#function-pointer-uart-abstraction)).

## Function-pointer UART abstraction

Every target sends bytes its own way: HAL transmit, Arduino `Serial.write`,
POSIX `fwrite`, a DMA queue. Rather than pick one, the library takes the
transport as a function pointer, `void (*uart_send)(const char *, size_t)`, with
an optional `void (*screen_clear)(void)`, and calls nothing else to produce
output.

That keeps it portable C99 with no target headers, and hands every platform
question (blocking versus DMA, buffer lifetime, ISR interaction, ANSI support,
locking) back to the caller, where the [integration guide](integration.md)
covers them. It also makes testing easy: the suite passes a fake `uart_send`
that captures output into a buffer, no hardware or mocks.
