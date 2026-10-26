#include "fast_ring_buffer.h"
#include <stdint.h>
#include <unity.h>

#define BUF_SIZE 32

static fast_ring_buffer s_ring;
static uint8_t          s_storage[BUF_SIZE];

void setUp(void) { fast_ring_buffer_init(&s_ring, s_storage, BUF_SIZE); }

void tearDown(void) { }

void test_fast_ring_buffer(void)
{
    uint8_t out[BUF_SIZE * 2];

    TEST_ASSERT_EQUAL_PTR(s_storage, s_ring.buffer);
    TEST_ASSERT_EQUAL(BUF_SIZE, s_ring.size);
    TEST_ASSERT_EQUAL(0, s_ring.head);
    TEST_ASSERT_EQUAL(0, s_ring.tail);

    const char* test1 = "test1";
    TEST_ASSERT_FALSE(fast_ring_buffer_push(&s_ring, (const uint8_t*)test1, BUF_SIZE + 2));
    for (int i = 0; i < 100; ++i) {
        TEST_ASSERT_TRUE(fast_ring_buffer_push(&s_ring, (const uint8_t*)test1, 5));
        TEST_ASSERT_EQUAL(5, fast_ring_buffer_pop(&s_ring, out, BUF_SIZE * 2));
        TEST_ASSERT_EQUAL_UINT8_ARRAY(test1, out, 5);
    }

    TEST_ASSERT_TRUE(fast_ring_buffer_push(&s_ring, (const uint8_t*)test1, 5));
    fast_ring_buffer_clear(&s_ring);
    TEST_ASSERT_EQUAL(0, fast_ring_buffer_pop(&s_ring, out, BUF_SIZE * 2));

    const char* test2 = "foo.barbar.bazbazbaz.";
    TEST_ASSERT_TRUE(fast_ring_buffer_push(&s_ring, (const uint8_t*)test2, 21));
    uint32_t p = fast_ring_buffer_find_byte_count_to(&s_ring, '.');
    TEST_ASSERT_EQUAL(4, p);
    TEST_ASSERT_EQUAL(4, fast_ring_buffer_pop(&s_ring, out, p));
    p = fast_ring_buffer_find_byte_count_to(&s_ring, '.');
    TEST_ASSERT_EQUAL(7, p);
    TEST_ASSERT_EQUAL(7, fast_ring_buffer_pop(&s_ring, out, p));
    p = fast_ring_buffer_find_byte_count_to(&s_ring, '.');
    TEST_ASSERT_EQUAL(10, p);
    TEST_ASSERT_EQUAL(10, fast_ring_buffer_pop(&s_ring, out, p));
    p = fast_ring_buffer_find_byte_count_to(&s_ring, '.');
    TEST_ASSERT_EQUAL(0, p);
}

void test_full_and_empty_boundaries(void)
{
    uint8_t data[BUF_SIZE + 1];
    uint8_t out[BUF_SIZE + 1];
    for (uint8_t i = 0; i < sizeof(data); ++i) {
        data[i] = i;
    }

    // Exactly full is fine, one more byte isn't; push is all or nothing.
    TEST_ASSERT_TRUE(fast_ring_buffer_push(&s_ring, data, BUF_SIZE - 1));
    TEST_ASSERT_FALSE(fast_ring_buffer_push(&s_ring, data, 2));
    TEST_ASSERT_EQUAL(BUF_SIZE - 1, fast_ring_buffer_used(&s_ring));
    TEST_ASSERT_TRUE(fast_ring_buffer_push(&s_ring, &data[BUF_SIZE - 1], 1));
    TEST_ASSERT_EQUAL(BUF_SIZE, fast_ring_buffer_used(&s_ring));
    TEST_ASSERT_FALSE(fast_ring_buffer_push(&s_ring, data, 1));
    // Zero-length push always succeeds.
    TEST_ASSERT_TRUE(fast_ring_buffer_push(&s_ring, data, 0));

    // Partial pops.
    TEST_ASSERT_EQUAL(10, fast_ring_buffer_pop(&s_ring, out, 10));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(data, out, 10);
    TEST_ASSERT_EQUAL(BUF_SIZE - 10, fast_ring_buffer_pop(&s_ring, out, sizeof(out)));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(&data[10], out, BUF_SIZE - 10);
    TEST_ASSERT_EQUAL(0, fast_ring_buffer_used(&s_ring));
    TEST_ASSERT_EQUAL(0, fast_ring_buffer_pop(&s_ring, out, sizeof(out)));
}

void test_counter_wraparound(void)
{
    // Start the free running counters just below the 32-bit wraparound.
    s_ring.head = UINT32_MAX - 5;
    s_ring.tail = UINT32_MAX - 5;

    const uint8_t data[12] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', '\r'};
    uint8_t       out[12];
    for (int round = 0; round < 10; ++round) {
        TEST_ASSERT_TRUE(fast_ring_buffer_push(&s_ring, data, sizeof(data)));
        TEST_ASSERT_EQUAL(sizeof(data), fast_ring_buffer_used(&s_ring));
        TEST_ASSERT_EQUAL(sizeof(data), fast_ring_buffer_find_byte_count_to(&s_ring, '\r'));
        TEST_ASSERT_EQUAL(sizeof(data), fast_ring_buffer_pop(&s_ring, out, sizeof(out)));
        TEST_ASSERT_EQUAL_UINT8_ARRAY(data, out, sizeof(data));
    }
}

void test_find_across_storage_wrap(void)
{
    uint8_t out[BUF_SIZE];
    uint8_t filler[BUF_SIZE - 4] = {0};
    // Move the indexes near the end of the storage.
    TEST_ASSERT_TRUE(fast_ring_buffer_push(&s_ring, filler, sizeof(filler)));
    TEST_ASSERT_EQUAL(sizeof(filler), fast_ring_buffer_pop(&s_ring, out, sizeof(out)));

    const uint8_t line[8] = {'1', '2', '3', '4', '5', '6', '7', '\r'};
    TEST_ASSERT_TRUE(fast_ring_buffer_push(&s_ring, line, sizeof(line)));
    TEST_ASSERT_EQUAL(8, fast_ring_buffer_find_byte_count_to(&s_ring, '\r'));
    TEST_ASSERT_EQUAL(0, fast_ring_buffer_find_byte_count_to(&s_ring, 'x'));
    TEST_ASSERT_EQUAL(8, fast_ring_buffer_pop(&s_ring, out, sizeof(out)));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(line, out, sizeof(line));
}

void test_skip(void)
{
    const uint8_t data[6] = {1, 2, 3, 4, 5, 6};
    uint8_t       out[6];
    TEST_ASSERT_TRUE(fast_ring_buffer_push(&s_ring, data, sizeof(data)));
    TEST_ASSERT_EQUAL(2, fast_ring_buffer_skip(&s_ring, 2));
    TEST_ASSERT_EQUAL(2, fast_ring_buffer_pop(&s_ring, out, 2));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(&data[2], out, 2);
    // Skipping more than available only skips what is there.
    TEST_ASSERT_EQUAL(2, fast_ring_buffer_skip(&s_ring, 100));
    TEST_ASSERT_EQUAL(0, fast_ring_buffer_used(&s_ring));
    TEST_ASSERT_EQUAL(0, fast_ring_buffer_skip(&s_ring, 1));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_fast_ring_buffer);
    RUN_TEST(test_full_and_empty_boundaries);
    RUN_TEST(test_counter_wraparound);
    RUN_TEST(test_find_across_storage_wrap);
    RUN_TEST(test_skip);
    return UNITY_END();
}
