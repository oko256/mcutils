#pragma once

// Host unit testing replacement for CMSIS compiler intrinsics.

#include <stdint.h>

/// Data memory barrier; on the host a compiler barrier is enough for single-threaded tests.
#define __DMB() __asm__ volatile("" ::: "memory")

/// Main stack pointer value returned by __get_MSP(), set by the tests.
extern uintptr_t g_stub_msp;

static inline uintptr_t __get_MSP(void) { return g_stub_msp; }
