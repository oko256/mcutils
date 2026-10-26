#include "stack_monitor.h"
#include <stdint.h>
#include <unity.h>

#define STACK_WORDS   64
#define STACK_BYTES   (STACK_WORDS * 4)
#define STACK_PATTERN 0xDEADBEEFu
#define FILL          0x11111111u

// Fake stack in place of the real one: the linker script symbols are defined to point to it,
// _estack at its end and absolute _Min_Stack_Size as its size.
uint32_t g_fake_stack[STACK_WORDS];
__asm__(".globl _estack\n"
        ".set _estack, g_fake_stack + 256\n"
        ".globl _Min_Stack_Size\n"
        ".set _Min_Stack_Size, 256\n");
_Static_assert(STACK_BYTES == 256, "must match the symbols above");

uintptr_t g_stub_msp;

/// Sets SP to point at the given word of the fake stack.
static void s_set_sp(uint32_t word) { g_stub_msp = (uintptr_t)&g_fake_stack[word]; }

void setUp(void)
{
    for (uint32_t i = 0; i < STACK_WORDS; i++) {
        g_fake_stack[i] = FILL;
    }
    s_set_sp(STACK_WORDS);
}

void tearDown(void) { }

void test_size(void) { TEST_ASSERT_EQUAL_UINT32(STACK_BYTES, stack_monitor_size()); }

void test_init_paints_only_below_sp(void)
{
    s_set_sp(48);
    stack_monitor_init();
    for (uint32_t i = 0; i < STACK_WORDS; i++) {
        TEST_ASSERT_EQUAL_HEX32_MESSAGE(i < 48 ? STACK_PATTERN : FILL, g_fake_stack[i], "word");
    }
    TEST_ASSERT_EQUAL_UINT32((STACK_WORDS - 48) * 4, stack_monitor_used());
    TEST_ASSERT_FALSE(stack_monitor_overflowed());
}

void test_nothing_used_when_painted_up_to_top(void)
{
    s_set_sp(STACK_WORDS);
    stack_monitor_init();
    TEST_ASSERT_EQUAL_UINT32(0, stack_monitor_used());
    TEST_ASSERT_FALSE(stack_monitor_overflowed());
}

void test_used_is_high_water_mark(void)
{
    s_set_sp(60);
    stack_monitor_init();

    // Stack grows down: a deeper call overwrites words below the SP at init.
    g_fake_stack[40] = 0;
    g_fake_stack[50] = 0;
    TEST_ASSERT_EQUAL_UINT32((STACK_WORDS - 40) * 4, stack_monitor_used());
    // Returning from the call doesn't lower the mark.
    g_fake_stack[50] = STACK_PATTERN;
    TEST_ASSERT_EQUAL_UINT32((STACK_WORDS - 40) * 4, stack_monitor_used());
    TEST_ASSERT_FALSE(stack_monitor_overflowed());
}

void test_pattern_valued_data_is_not_counted(void)
{
    // Documented limitation: data equal to the pattern at the deepest used word looks unused.
    s_set_sp(60);
    stack_monitor_init();
    g_fake_stack[30] = STACK_PATTERN;
    g_fake_stack[31] = 0;
    TEST_ASSERT_EQUAL_UINT32((STACK_WORDS - 31) * 4, stack_monitor_used());
}

void test_lowest_word_used_is_overflow(void)
{
    s_set_sp(60);
    stack_monitor_init();
    g_fake_stack[1] = 0;
    TEST_ASSERT_FALSE(stack_monitor_overflowed());
    g_fake_stack[0] = 0;
    TEST_ASSERT_TRUE(stack_monitor_overflowed());
    TEST_ASSERT_EQUAL_UINT32(STACK_BYTES, stack_monitor_used());
}

void test_whole_stack_used(void)
{
    // Nothing painted, e.g. init called with SP at the bottom.
    s_set_sp(0);
    stack_monitor_init();
    TEST_ASSERT_EQUAL_HEX32(FILL, g_fake_stack[0]);
    TEST_ASSERT_EQUAL_UINT32(STACK_BYTES, stack_monitor_used());
    TEST_ASSERT_TRUE(stack_monitor_overflowed());
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_size);
    RUN_TEST(test_init_paints_only_below_sp);
    RUN_TEST(test_nothing_used_when_painted_up_to_top);
    RUN_TEST(test_used_is_high_water_mark);
    RUN_TEST(test_pattern_valued_data_is_not_counted);
    RUN_TEST(test_lowest_word_used_is_overflow);
    RUN_TEST(test_whole_stack_used);
    return UNITY_END();
}
