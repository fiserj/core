#include <stddef.h> // uintptr_t
#include <stdio.h>  // freopen, stderr
#include <string.h> // memcmp

#include <utest.h> // ASSERT_*, EXPECT_EXCEPTION, UTEST_*

#include "core.h"

// -----------------------------------------------------------------------------
// DEFERRED EXECUTION
// -----------------------------------------------------------------------------

UTEST(defer, order) {
  int val = 1;

  defer(ASSERT_EQ(val++, 3));
  defer(ASSERT_EQ(val++, 2));
  defer(ASSERT_EQ(val++, 1));
}

// -----------------------------------------------------------------------------
// SMALL UTILITIES
// -----------------------------------------------------------------------------

UTEST(utils, min) {
  ASSERT_EQ(min(1, 1), 1);
  ASSERT_EQ(min(1, 3), 1);
  ASSERT_EQ(min(3, 1), 1);

  ASSERT_EQ(min(1.0, 1.0), 1.0);
  ASSERT_EQ(min(1.0, 3.0), 1.0);
  ASSERT_EQ(min(3.0, 1.0), 1.0);
}

UTEST(utils, max) {
  ASSERT_EQ(max(1, 1), 1);
  ASSERT_EQ(max(1, 3), 3);
  ASSERT_EQ(max(3, 1), 3);

  ASSERT_EQ(max(1.0, 1.0), 1.0);
  ASSERT_EQ(max(1.0, 3.0), 3.0);
  ASSERT_EQ(max(3.0, 1.0), 3.0);
}

UTEST(utils, clamp) {
  ASSERT_EQ(clamp(-1, 0, 2), 0);
  ASSERT_EQ(clamp(+0, 0, 2), 0);
  ASSERT_EQ(clamp(+1, 0, 2), 1);
  ASSERT_EQ(clamp(+2, 0, 2), 2);
  ASSERT_EQ(clamp(+3, 0, 2), 2);
}

UTEST(utils, is_power_of_two) {
  ASSERT_TRUE(is_power_of_two(0));
  ASSERT_TRUE(is_power_of_two(1));
  ASSERT_TRUE(is_power_of_two(2));
  ASSERT_TRUE(is_power_of_two(4));
  ASSERT_TRUE(is_power_of_two(128));

  ASSERT_FALSE(is_power_of_two(3));
  ASSERT_FALSE(is_power_of_two(5));
  ASSERT_FALSE(is_power_of_two(22));
  ASSERT_FALSE(is_power_of_two(50));
  ASSERT_FALSE(is_power_of_two(127));
}

UTEST(utils, align_up) {
  ASSERT_EQ(align_up(0, 4), 0);
  ASSERT_EQ(align_up(1, 4), 4);
  ASSERT_EQ(align_up(2, 4), 4);
  ASSERT_EQ(align_up(3, 4), 4);
  ASSERT_EQ(align_up(4, 4), 4);

  ASSERT_EQ(align_up(16, 32), 32);
  ASSERT_EQ(align_up(16, 64), 64);
  ASSERT_EQ(align_up(16, 128), 128);
}

// -----------------------------------------------------------------------------
// ALLOCATORS
// -----------------------------------------------------------------------------

UTEST(std_alloc, zeroed_memory) {
  const u8 zeros[128] = {};

  Allocator alloc = std_alloc();

  constexpr Size size = 13;

  void* mem = allocate(alloc, nullptr, 0, size, 1);
  defer(allocate(alloc, mem, size, 0, 0));

  ASSERT_EQ(memcmp(mem, zeros, size), 0);
}

UTEST(std_alloc, alignment) {
  Allocator alloc = std_alloc();

  constexpr Size  size     = 13;
  const uintptr_t aligns[] = {1, 4, 16, 32, 64};

  // NOTE : Just because `ASSERT_EQ` spits out warnings when `%` is used.
  const auto mod = [](auto _x, auto _y) -> Size {
    return _x % _y;
  };

  for (auto align : aligns) {
    void* mem = allocate(alloc, nullptr, 0, size, align);
    defer(allocate(alloc, mem, size, 0, 0));

    ASSERT_EQ(mod(uintptr_t(mem), align), 0);
  }
}

UTEST(Arena, make_arena) {
  u8 buf[32] = {};

  Arena arena = make_arena(make_slice(buf));

  ASSERT_EQ(arena.first, buf);
  ASSERT_EQ(arena.last, buf + sizeof(buf));
}

UTEST(arena_alloc, zeroed_memory) {
  const u8 zeros[128] = {};

  u8 buf[128];
  memset(buf, 0xff, sizeof(buf));

  Arena     arena = make_arena(make_slice(buf));
  Allocator alloc = make_arena_alloc(arena);

  constexpr Size size = 13;

  void* mem = allocate(alloc, nullptr, 0, size, 1);

  ASSERT_EQ(memcmp(mem, zeros, size), 0);
}

UTEST(arena_alloc, alignment) {
  u8 buf[128];

  constexpr Size  size     = 13;
  const uintptr_t aligns[] = {1, 4, 16, 32, 64};

  // NOTE : Just because `ASSERT_EQ` spits out warnings when `%` is used.
  const auto mod = [](auto _x, auto _y) -> Size {
    return _x % _y;
  };

  for (auto align : aligns) {
    Arena     arena = make_arena(make_slice(buf));
    Allocator alloc = make_arena_alloc(arena);

    void* mem = allocate(alloc, nullptr, 0, size, align);

    ASSERT_EQ(mod(uintptr_t(mem), align), 0);
  }
}

// -----------------------------------------------------------------------------
// SLICE
// -----------------------------------------------------------------------------

UTEST(Slice, truthiness) {
  int array[3] = {1, 2, 3};

  const detail::Slice<int> valid = {
    .data = array,
    .len  = sizeof(array) / sizeof(array[0]),
  };

  const detail::Slice<int> empty = {};

  ASSERT_TRUE(valid);
  ASSERT_FALSE(empty);
}

UTEST(Slice, element_access) {
  int array[3] = {1, 2, 3};

  const detail::Slice<int> slice = {
    .data = array,
    .len  = sizeof(array) / sizeof(array[0]),
  };

  ASSERT_EQ(slice[0], 1);
  ASSERT_EQ(slice[1], 2);
  ASSERT_EQ(slice[2], 3);

  EXPECT_EXCEPTION(slice[-1], int);
  EXPECT_EXCEPTION(slice[+4], int);
}

UTEST(Slice, subslicing) {
  int array[3] = {1, 2, 3};

  const detail::Slice<int> slice = {
    .data = array,
    .len  = sizeof(array) / sizeof(array[0]),
  };

  ASSERT_EQ(slice(0, 2).len, 2);
  ASSERT_EQ(slice(_, 2).len, 2);
  ASSERT_EQ(slice(1, _).len, 2);

  EXPECT_EXCEPTION(slice(-1, 2), int);
  EXPECT_EXCEPTION(slice(2, 1), int);
  EXPECT_EXCEPTION(slice(_, 4), int);
}

UTEST(make_slice, data_len) {
  int val = 1;

  auto slice = make_slice(&val, 1);

  ASSERT_EQ(slice.data, &val);
  ASSERT_EQ(slice.len, 1);
}

UTEST(make_slice, array) {
  int array[3] = {1, 2, 3};

  auto slice = make_slice(array);

  ASSERT_EQ(slice.data, array);
  ASSERT_EQ(slice.len, 3);
}

UTEST(make_slice, len) {
  auto slice = make_slice<int>(3);
  defer(destroy(slice));

  ASSERT_NE(slice.data, (int*)nullptr);
  ASSERT_EQ(slice.len, 3);
  ASSERT_LE(slice.len, slice.cap);
}

UTEST(make_slice, len_cap) {
  auto slice = make_slice<int>(1, 3);
  defer(destroy(slice));

  ASSERT_NE(slice.data, (int*)nullptr);
  ASSERT_EQ(slice.len, 1);
  ASSERT_EQ(slice.cap, 3);
}

// -----------------------------------------------------------------------------
// MAIN
// -----------------------------------------------------------------------------

UTEST_STATE();

int main(int _argc, char** _argv) {
  freopen("/dev/null", "w", stderr);

  return utest_main(_argc, _argv);
}
