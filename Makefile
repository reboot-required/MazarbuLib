# Copyright (c) 2026 Lukas Kraft
# https://github.com/reboot-required
#
# Part of MazarbuLib — a UART screen display library for embedded systems.
# Named after the Book of Mazarbul from J.R.R. Tolkien's writings.
#
# SPDX-License-Identifier: MIT

CC  ?= gcc
AR  ?= ar

CFLAGS  ?= -std=c99 -Wall -Wextra -Wpedantic
CFLAGS  += -Iinclude

SRC := src/mazarbulib.c
OBJ := $(SRC:.c=.o)
LIB := libmazarbulib.a

.PHONY: all clean

all: $(LIB)

$(LIB): $(OBJ)
	$(AR) rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	$(RM) $(OBJ) $(LIB)
