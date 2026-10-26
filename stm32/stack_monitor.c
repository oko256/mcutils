/**
 * SPDX-License-Identifier: MIT
 * Copyright 2026 Olli Kokko <https://github.com/oko256/mcutils>
 */

#include "stack_monitor.h"
#include <cmsis_compiler.h>

#define STACK_PATTERN 0xDEADBEEFu

extern uint32_t _estack; // address = top of stack
extern uint32_t _Min_Stack_Size; // absolute symbol: address = actual value

#define STACK_TOP    ((uint32_t*)&_estack)
#define STACK_SIZE   ((uintptr_t)&_Min_Stack_Size)
#define STACK_BOTTOM ((uint32_t*)((uintptr_t)&_estack - STACK_SIZE))

__attribute__((noinline)) void stack_monitor_init(void)
{
    // Main stack is the active stack at the start of main().
    const uintptr_t sp = __get_MSP();

    // Paint everything below the current SP. Safe as long as no interrupt is active yet,
    // which is true at the very start of main().
    for (uint32_t* p = STACK_BOTTOM; p < (uint32_t*)sp; p++) {
        *p = STACK_PATTERN;
    }
}

uint32_t stack_monitor_used(void)
{
    const uint32_t* p = STACK_BOTTOM;
    while (p < STACK_TOP && *p == STACK_PATTERN) {
        p++;
    }
    return (uint32_t)((uintptr_t)STACK_TOP - (uintptr_t)p);
}

uint32_t stack_monitor_size(void) { return (uint32_t)STACK_SIZE; }

bool stack_monitor_overflowed(void) { return *STACK_BOTTOM != STACK_PATTERN; }
