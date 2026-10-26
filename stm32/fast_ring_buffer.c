#include "fast_ring_buffer.h"
#include <cmsis_compiler.h>

// Memory barriers order the buffer data accesses against the head/tail index updates, so that the
// other side never sees an index update before the data it refers to has been written (or read).

void fast_ring_buffer_init(fast_ring_buffer* self, uint8_t* buffer, uint32_t size)
{
    // Size must be a power of 2.
    self->buffer = buffer;
    self->size   = size;
    self->head   = 0;
    self->tail   = 0;
}

void fast_ring_buffer_clear(fast_ring_buffer* self)
{
    self->head = 0;
    self->tail = 0;
}

uint32_t fast_ring_buffer_used(const fast_ring_buffer* self) { return self->head - self->tail; }

uint32_t fast_ring_buffer_find_byte_count_to(const fast_ring_buffer* self, uint8_t ch)
{
    const uint32_t current_tail = self->tail;
    const uint32_t current_head = self->head;
    const uint32_t used_space   = current_head - current_tail;
    // Data must not be read before the head that publishes it.
    __DMB();

    if (used_space == 0 || used_space > self->size) {
        // Buffer is empty, or invalid state, corrupted counters, or a synch bug.
        return 0;
    }

    for (uint32_t i = 0; i < used_space; ++i) {
        uint32_t idx = (current_tail + i) & (self->size - 1);
        if (self->buffer[idx] == ch) {
            // Found.
            return (i + 1);
        }
    }

    // Not found.
    return 0;
}

bool fast_ring_buffer_push(fast_ring_buffer* self, const uint8_t* data, uint32_t len)
{
    const uint32_t current_tail = self->tail;
    const uint32_t current_head = self->head;
    const uint32_t used_space   = current_head - current_tail;

    if (used_space > self->size) {
        // Invalid state, corrupted counters, or a synch bug.
        return false;
    }

    // How much free space in the ring?
    const uint32_t space_available = self->size - used_space;
    if (len > space_available) {
        // Not enough space in the buffer.
        return false;
    }

    // Consumer must have finished reading the space before we overwrite it.
    __DMB();

    // Push into ring.
    for (uint32_t i = 0; i < len; ++i) {
        uint32_t idx      = (current_head + i) & (self->size - 1);
        self->buffer[idx] = data[i];
    }

    // Data must be written before it's published to the consumer.
    __DMB();
    self->head = current_head + len;

    return true;
}

uint32_t fast_ring_buffer_pop(fast_ring_buffer* self, uint8_t* out_data, uint32_t max_len)
{
    const uint32_t current_tail = self->tail;
    const uint32_t current_head = self->head;
    // Data must not be read before the head that publishes it.
    __DMB();

    // How many bytes are in the ring?
    const uint32_t bytes_available = current_head - current_tail;
    const uint32_t bytes_to_copy   = (bytes_available > max_len) ? max_len : bytes_available;

    // Copy from the ring buffer to the output buffer (handling ring wraparound).
    for (uint32_t i = 0; i < bytes_to_copy; ++i) {
        uint32_t idx = (current_tail + i) & (self->size - 1);
        out_data[i]  = self->buffer[idx];
    }

    // Data must be read before the space is released to the producer.
    __DMB();
    self->tail = current_tail + bytes_to_copy;

    return bytes_to_copy;
}

uint32_t fast_ring_buffer_skip(fast_ring_buffer* self, uint32_t len)
{
    const uint32_t current_tail    = self->tail;
    const uint32_t bytes_available = self->head - current_tail;
    const uint32_t bytes_to_skip   = (bytes_available > len) ? len : bytes_available;
    self->tail                     = current_tail + bytes_to_skip;
    return bytes_to_skip;
}
