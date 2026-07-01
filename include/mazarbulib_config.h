// Copyright (c) 2026 Lukas Kraft
// https://github.com/reboot-required
//
// Part of MazarbuLib — a UART screen display library for embedded systems.
// Named after the Book of Mazarbul from J.R.R. Tolkien's writings.
//
// SPDX-License-Identifier: MIT

#ifndef MAZARBULIB_INCLUDE_MAZARBULIB_CONFIG_H_
#define MAZARBULIB_INCLUDE_MAZARBULIB_CONFIG_H_

// Maximum number of screens that can be registered.
#ifndef MAZARBULIB_MAX_SCREENS
#define MAZARBULIB_MAX_SCREENS 8
#endif

// Maximum number of rows per screen.
#ifndef MAZARBULIB_MAX_ROWS_PER_SCREEN
#define MAZARBULIB_MAX_ROWS_PER_SCREEN 16
#endif

// Width of the label column in characters, excluding table borders.
#ifndef MAZARBULIB_LABEL_WIDTH
#define MAZARBULIB_LABEL_WIDTH 20
#endif

// Width of the value column in characters, excluding table borders.
#ifndef MAZARBULIB_VALUE_WIDTH
#define MAZARBULIB_VALUE_WIDTH 15
#endif

// UART byte that navigates to the next screen.
#ifndef MAZARBULIB_NAV_NEXT
#define MAZARBULIB_NAV_NEXT 'n'
#endif

// UART byte that navigates to the previous screen.
#ifndef MAZARBULIB_NAV_PREV
#define MAZARBULIB_NAV_PREV 'p'
#endif

#endif  // MAZARBULIB_INCLUDE_MAZARBULIB_CONFIG_H_
