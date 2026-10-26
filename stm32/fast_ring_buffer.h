/**
 * @file
 * @brief Single-producer single-consumer ring buffer.
 *
 * Producer and consumer may run in different contexts (e.g. interrupt and main loop) without
 * locking.
 *
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 oko256 <https://github.com/oko256/mcutils>
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint8_t*          buffer;
    uint32_t          size;
    volatile uint32_t head;
    volatile uint32_t tail;
} fast_ring_buffer;

/// Initializes ring buffer that uses given buffer as storage. Size must be a power of 2.
void fast_ring_buffer_init(fast_ring_buffer* self, uint8_t* buffer, uint32_t size);

/// Empties the ring buffer. Only call when neither producer nor consumer can be active.
void fast_ring_buffer_clear(fast_ring_buffer* self);

/// Returns how many bytes are currently stored in the ring.
uint32_t fast_ring_buffer_used(const fast_ring_buffer* self);

/// Returns how many bytes to read to get to first of the specified bytes in the ring, or 0 if not found.
/// Consumer side.
uint32_t fast_ring_buffer_find_byte_count_to(const fast_ring_buffer* self, uint8_t ch);

/// Pushes new data to ring buffer. Copies all or nothing; returns true if managed to copy.
/// Producer side: only push from one context at a time!
bool fast_ring_buffer_push(fast_ring_buffer* self, const uint8_t* data, uint32_t len);

/// Pops data out of ring buffer up to max size of output buffer. Returns number of bytes copied.
/// Consumer side: only pop from one context at a time!
uint32_t fast_ring_buffer_pop(fast_ring_buffer* self, uint8_t* out_data, uint32_t max_len);

/// Discards up to len bytes from the ring buffer. Returns number of bytes discarded.
/// Consumer side: only skip from the same context that pops!
uint32_t fast_ring_buffer_skip(fast_ring_buffer* self, uint32_t len);
