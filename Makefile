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

SRC := src/mazarbulib.c
OBJ := $(SRC:.c=.o)
LIB := libmazarbulib.a

EXAMPLE_SRC := examples/posix.c
EXAMPLE_BIN := mazarbulib_posix_demo

TEST_SRC := tests/test_mazarbulib.c
TEST_BIN := mazarbulib_test

.PHONY: all clean posix-example test

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

clean:
	$(RM) $(OBJ) $(LIB) $(EXAMPLE_BIN) $(TEST_BIN)
