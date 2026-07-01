# Copyright (c) 2026 Lukas Kraft
# https://github.com/reboot-required
#
# Part of MazarbuLib — a UART screen display library for embedded systems.
# Named after the Book of Mazarbul from J.R.R. Tolkien's writings.
#
# SPDX-License-Identifier: MIT

CC  ?= gcc
AR  ?= ar

# CFLAGS is overridable (CI adds -Werror); the library's include path lives in
# CPPFLAGS so overriding CFLAGS never drops it.
CFLAGS   ?= -std=c99 -Wall -Wextra -Wpedantic
CPPFLAGS += -Iinclude

SANITIZERS := -fsanitize=address,undefined -fno-sanitize-recover=all

SRC := src/mazarbulib.c
OBJ := $(SRC:.c=.o)
LIB := libmazarbulib.a

EXAMPLE_SRC := examples/posix.c
EXAMPLE_BIN := mazarbulib_posix_demo

TEST_SRC := tests/test_mazarbulib.c
TEST_BIN := mazarbulib_test

.PHONY: all clean posix-example test format analyze asan

all: $(LIB)

$(LIB): $(OBJ)
	$(AR) rcs $@ $^

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

posix-example: $(SRC) $(EXAMPLE_SRC)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SRC) $(EXAMPLE_SRC) -o $(EXAMPLE_BIN)

test: $(SRC) $(TEST_SRC)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SRC) $(TEST_SRC) -o $(TEST_BIN)
	./$(TEST_BIN)

# Build and run the test suite instrumented with ASan + UBSan (host only).
asan: $(SRC) $(TEST_SRC)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SANITIZERS) -g $(SRC) $(TEST_SRC) \
		-o $(TEST_BIN)
	./$(TEST_BIN)

# Rewrite all sources in place to match .clang-format.
format:
	clang-format -i src/*.c include/*.h tests/*.c examples/*.c examples/*.cpp

# Run cppcheck over the buildable host sources.
analyze:
	cppcheck --std=c99 --enable=warning,style,performance,portability \
		--error-exitcode=1 $(CPPFLAGS) $(SRC) $(TEST_SRC) $(EXAMPLE_SRC)

clean:
	$(RM) $(OBJ) $(LIB) $(EXAMPLE_BIN) $(TEST_BIN)
