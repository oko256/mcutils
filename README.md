# mcutils - microcontroller utilities

Small, dependency-free C utilities for microcontroller firmware. Each module is a `.c`/`.h` pair
that you add to your firmware build directly; there is no library to build or configure.

| Directory | Contents |
|---|---|
| `stm32/` | Utilities for STM32 (Cortex-M) with CMSIS and the STM32CubeMX GCC linker script |

## stm32

### `fast_ring_buffer`

A lock-free single-producer single-consumer ring buffer for passing bytes between, for example, an
interrupt and the main loop without disabling interrupts.

```c
static uint8_t          s_storage[128]; // size must be a power of 2
static fast_ring_buffer s_ring;

fast_ring_buffer_init(&s_ring, s_storage, sizeof(s_storage));
fast_ring_buffer_push(&s_ring, data, len);           // producer
n = fast_ring_buffer_pop(&s_ring, out, sizeof(out)); // consumer
```

- **Size** must be a power of 2.
- **Contexts:**
  - `push` is the producer side.
  - `pop`, `skip` and `find_byte_count_to` are the consumer side.
  - Each side may be used from one context only.
  - `clear` and `init` need both sides to be idle.
- **Line-based input:** `find_byte_count_to(ch)` returns the number of bytes up to and including the
  first `ch`, or 0 if `ch` isn't in the buffer. This is convenient when processing the ring buffer
  contents piece by piece delimited by certain byte like `\n` or `\r`.
- **Memory ordering:** `__DMB()` barriers order the data accesses against the index updates.
- **Requires:** `cmsis_compiler.h` (for `__DMB()`), which comes with CMSIS.

### `stack_monitor`

Measures the worst-case stack usage by stack painting:
1. At startup, `stack_monitor_init()` fills the unused stack below the current SP with a pattern.
2. Later, `stack_monitor_used()` finds the lowest word that is no longer the pattern.

```c
int main(void)
{
    stack_monitor_init(); // first thing in main(), before interrupts are enabled
    ...
}

// At later point in program, or from a debug command:
uint32_t used = stack_monitor_used(); // high-water mark, i.e. max used bytes at any point
uint32_t size = stack_monitor_size(); // the total stack size in bytes
bool overflow = stack_monitor_overflowed(); // returns true if no pattern exists at all in stack anymore
```

- **Linker symbols:** requires the `_estack` (top of the stack) and absolute `_Min_Stack_Size`
  symbols that the STM32CubeMX GCC linker scripts define. The stack is assumed to be the
  `_Min_Stack_Size` bytes below `_estack`.
- **SP:** read with CMSIS `__get_MSP()`. Call `stack_monitor_init()` while running on the main
  stack, i.e. from `main()` before an RTOS or any interrupt starts.
- The result is a high-water mark of what has been exercised so far. Run the most
  stack-hungry code paths (and interrupts) before reading it.
- A stored value that happens to equal the pattern (`0xDEADBEEF`) at the deepest point makes the
  usage look smaller, by one word at a time.
- `stack_monitor_overflowed()` only reports that the lowest stack word was written. A real overflow
  may already have corrupted the memory below the stack.

## Unit tests

Unit tests are in `*/test/` directories, which are CMake projects that compile the modules for the
build host with some stubbing and other tricks to make them unit testable on a regular PC.
Unit tests are built with sanitizers enabled to catch mistakes.

```sh
cd stm32/test
cmake --preset test && cmake --build --preset test && ctest --preset test
```

### Coverage report

Requires GCC, gcov, [fastcov](https://github.com/RPGillespie6/fastcov) (`pip install fastcov`) and
`genhtml` (from lcov).

```sh
cd stm32/test
cmake --preset coverage && cmake --build --preset coverage
# Open stm32/test/build-coverage/coverage/index.html
```
