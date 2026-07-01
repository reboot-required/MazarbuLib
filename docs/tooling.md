# Tooling Guide

How to build, test, analyse, and format MazarbuLib. Two build systems are
provided and kept in sync: CMake for consumers and packaging, and a plain
Makefile for quick host-side work. Everything below runs from the repository
root.

## Building

### CMake

```bash
cmake -B build
cmake --build build
```

Options (pass with `-D<name>=ON`):

| Option | Default | Effect |
|--------|---------|--------|
| `MAZARBULIB_BUILD_TESTS` | `OFF` | Build the test binary and register CTest cases. |
| `MAZARBULIB_BUILD_EXAMPLES` | `OFF` | Build the POSIX demo executable. |
| `MAZARBULIB_WARNINGS_AS_ERRORS` | `OFF` | Add `-Werror` to first-party targets (CI uses this). |
| `MAZARBULIB_ENABLE_SANITIZERS` | `OFF` | Build first-party targets with ASan + UBSan. |

`-Werror` is opt-in so a newer compiler's warnings never break consumers who
build the library; CI turns it on explicitly.

### Makefile

The Makefile targets cover the common host tasks:

| Target | What it does |
|--------|--------------|
| `make` | Build `libmazarbulib.a`. |
| `make posix-example` | Build `mazarbulib_posix_demo`. |
| `make test` | Build and run the test suite. |
| `make asan` | Build and run the tests under ASan + UBSan. |
| `make format` | Rewrite all sources in place to match `.clang-format`. |
| `make analyze` | Run cppcheck over the host sources. |
| `make clean` | Remove build artifacts. |

`CFLAGS` is overridable; the include path lives in `CPPFLAGS` so overriding
`CFLAGS` never drops it. CI builds with:

```bash
make test CFLAGS="-std=c99 -Wall -Wextra -Wpedantic -Werror"
```

## Tests

The suite in [`tests/test_mazarbulib.c`](../tests/test_mazarbulib.c) is
self-contained with no external framework. A fake `uart_send` captures output
into a buffer that assertions inspect.

Run the whole suite:

```bash
make test
# or, via CMake:
cmake -B build -DMAZARBULIB_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

The binary also runs a single test by name, which is how CTest registers each
case individually:

```bash
./mazarbulib_test test_navigation
```

### Adding a test

1. Write a `static void test_<name>(void)` using `TEST_ASSERT(cond)`.
2. Add an entry to the `k_tests[]` table so the runner picks it up.
3. Register it in `CMakeLists.txt` with `add_test(NAME test_<name> ...)` so
   CTest runs it too.

## Sanitizers

AddressSanitizer and UndefinedBehaviorSanitizer run on the host only and are
never part of the shipped embedded build. They instrument both the library and
the test objects:

```bash
make asan
# or, via CMake:
cmake -B build -DMAZARBULIB_BUILD_TESTS=ON -DMAZARBULIB_ENABLE_SANITIZERS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

`-fno-sanitize-recover=all` makes any finding abort the run so CI fails loudly.

## Static analysis

Two analysers run over the host sources.

```bash
make analyze          # cppcheck
```

cppcheck runs with `--enable=warning,style,performance,portability` and
`--error-exitcode=1`. clang-tidy is driven from a compile database:

```bash
cmake -B build -DMAZARBULIB_BUILD_TESTS=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
clang-tidy -p build src/mazarbulib.c
```

The check set in [`.clang-tidy`](../.clang-tidy) is deliberately conservative:
`bugprone-*`, `clang-analyzer-*`, `performance-*`, `portability-*`, and
`misc-*`, minus a few checks that clash with the codebase's intentional idioms
(short local names, the compile-time-guard typedefs, deliberate truncation).

## Formatting

Formatting is enforced. The style in [`.clang-format`](../.clang-format) is
Google C++ applied to C, tuned so pointers bind to the name, include blocks are
preserved as written, and case labels are not extra-indented.

```bash
make format                                   # rewrite in place
clang-format --dry-run --Werror src/*.c \
  include/*.h tests/*.c examples/*.c examples/*.cpp   # check only
```

## Continuous integration

[`.github/workflows/ci.yml`](../.github/workflows/ci.yml) runs on every push and
pull request:

| Job | What it checks |
|-----|----------------|
| `format` | Every source file matches `.clang-format`. |
| `build` | GCC and Clang matrix, both build systems, warnings as errors. |
| `sanitizers` | The test suite under ASan + UBSan. |
| `static-analysis` | cppcheck and clang-tidy. Advisory (`continue-on-error`) until the findings are baselined, then promoted to required. |

Reproduce any job locally with the commands above; the CI steps are thin
wrappers around the same CMake and Makefile invocations.
