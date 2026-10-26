/**
 * @file
 * @brief Simple stack usage monitoring using stack painting.
 *
 * These functions can be used to track stack usage by first painting the whole reserved stack
 * space with a magic pattern, and then you can query how much of the magic pattern is still
 * left in the reserved stack space.
 *
 * When using this method for checking the worst-case stack usage, you need to exercise the most
 * stack-hogging functionality of your program to see a true worst-case usage.
 *
 * @note This gives only an estimated high-water mark of the used stack space. Also, if magic pattern
 * happens to be actual data values that the application has stored when stack usage was high, the
 * estimate could be wrong.
 *
 * SPDX-License-Identifier: MIT
 * Copyright 2026 Olli Kokko <https://github.com/oko256/mcutils>
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

/// Paints the stack with magic pattern. Call this as the first thing in main() so that no
/// interrupt is active yet and we get the most accurate painting of the stack.
void stack_monitor_init(void);

/// Returns the stack usage high-water mark in bytes.
uint32_t stack_monitor_used(void);

/// Returns the reserved stack size in bytes.
uint32_t stack_monitor_size(void);

/// Returns true if the lowest word in reserved stack was hit.
bool stack_monitor_overflowed(void);
