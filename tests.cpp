#if _MSC_VER
#  define _CRT_SECURE_NO_WARNINGS
#endif

#include <stddef.h> // uintptr_t
#include <stdio.h>  // freopen, stderr
#include <string.h> // memcmp, memset

#include <utest.h> // ASSERT_*, EXPECT_EXCEPTION, UTEST_*

#include "core.h"

// -----------------------------------------------------------------------------
// COMMON
// -----------------------------------------------------------------------------

static const u8 ZERO_MEM[1024] = {};

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
  Allocator alloc = std_alloc();

  constexpr Size size = 13;

  void* mem = allocate(alloc, nullptr, 0, size, 1);
  defer(allocate(alloc, mem, size, 0, 0));

  ASSERT_EQ(memcmp(mem, ZERO_MEM, size), 0);
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
  u8 buf[128];
  memset(buf, 0xff, sizeof(buf));

  Arena     arena = make_arena(make_slice(buf));
  Allocator alloc = make_arena_alloc(arena);

  constexpr Size size = 13;

  void* mem = allocate(alloc, nullptr, 0, size, 1);

  ASSERT_EQ(memcmp(mem, ZERO_MEM, size), 0);
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

UTEST(arena_alloc, out_of_memory) {
  u8 buf[128];

  Arena     arena = make_arena(make_slice(buf));
  Allocator alloc = make_arena_alloc(arena);

  constexpr Size size = sizeof(buf) + 1;

  EXPECT_EXCEPTION(allocate(alloc, nullptr, 0, size, 1), int);
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
  // GCC is too smart and realizes we're effectively trying to do `ZERO_MEM[-1]`
  // in one of the tests (with the negative low index).
  //
  // On some GCC versions, `#pragma GCC diagnostic ignored "-Warray-bounds"`
  // is still ignored, so instead, we point the slice's start to `ZERO_MEM + 1`.
  const detail::Slice<const u8> slice = {
    .data = ZERO_MEM + 1,
    .len  = 3,
  };

  ASSERT_EQ(slice(0, 2).len, 2);
  ASSERT_EQ(slice(_, 2).len, 2);
  ASSERT_EQ(slice(1, _).len, 2);

  EXPECT_EXCEPTION(slice(-1, 2), int);
  EXPECT_EXCEPTION(slice(2, 1), int);
  EXPECT_EXCEPTION(slice(_, 4), int);
}

UTEST(Slice, make_slice_data_len) {
  int val = 1;

  auto slice = make_slice(&val, 1);

  ASSERT_EQ(slice.data, &val);
  ASSERT_EQ(slice.len, 1);
}

UTEST(Slice, make_slice_array) {
  int array[3] = {1, 2, 3};

  auto slice = make_slice(array);

  ASSERT_EQ(slice.data, array);
  ASSERT_EQ(slice.len, 3);
}

UTEST(Slice, make_slice_len) {
  auto slice = make_slice<int>(3);
  defer(destroy(slice));

  ASSERT_NE(slice.data, (int*)nullptr);
  ASSERT_EQ(slice.len, 3);
  ASSERT_LE(slice.len, slice.cap);
}

UTEST(Slice, make_slice_len_cap) {
  auto slice = make_slice<int>(1, 3);
  defer(destroy(slice));

  ASSERT_NE(slice.data, (int*)nullptr);
  ASSERT_EQ(slice.len, 1);
  ASSERT_EQ(slice.cap, 3);
}

UTEST(Slice, reserve) {
  auto slice = make_slice<int>(1, 2);
  defer(destroy(slice));

  slice[0] = 10;

  ASSERT_EQ(slice.cap, 2);
  ASSERT_EQ(slice.len, 1);
  ASSERT_EQ(slice[0], 10);

  reserve(slice, 6);
  ASSERT_GE(slice.cap, 6);
  ASSERT_EQ(slice.len, 1);
  ASSERT_EQ(slice[0], 10);

  reserve(slice, 25);
  ASSERT_GE(slice.cap, 25);
  ASSERT_EQ(slice.len, 1);
  ASSERT_EQ(slice[0], 10);
}

UTEST(Slice, resize) {
  const int values[10] = {0, 1, 2};

  auto slice = make_slice<int>(1);
  defer(destroy(slice));

  ASSERT_EQ(slice.len, 1);
  ASSERT_LE(slice.len, slice.cap);
  ASSERT_EQ(memcmp(slice.data, ZERO_MEM, 1 * sizeof(int)), 0);

  resize(slice, 3);
  ASSERT_EQ(slice.len, 3);
  ASSERT_LE(slice.len, slice.cap);
  ASSERT_EQ(memcmp(slice.data, ZERO_MEM, 3 * sizeof(int)), 0);

  slice[1] = 1;
  slice[2] = 2;
  ASSERT_EQ(memcmp(slice.data, values, 3 * sizeof(int)), 0);

  resize(slice, 10);
  ASSERT_EQ(slice.len, 10);
  ASSERT_LE(slice.len, slice.cap);
  ASSERT_EQ(memcmp(slice.data, values, 10 * sizeof(int)), 0);

  resize(slice, 1);
  ASSERT_EQ(slice.len, 1);
  ASSERT_LE(slice.len, slice.cap);
  ASSERT_EQ(memcmp(slice.data, ZERO_MEM, 1 * sizeof(int)), 0);

  resize(slice, 3);
  ASSERT_EQ(slice.len, 3);
  ASSERT_LE(slice.len, slice.cap);
  ASSERT_EQ(memcmp(slice.data, ZERO_MEM, 3 * sizeof(int)), 0);
}

UTEST(Slice, copy) {
  auto src = make_slice<int>(3);
  defer(destroy(src));
  src[0] = 1;
  src[1] = 2;
  src[2] = 3;

  auto dst = make_slice<int>(3);
  defer(destroy(dst));
  copy(dst, src);
  ASSERT_EQ(memcmp(src.data, dst.data, 3 * sizeof(int)), 0);

  int array[3] = {};
  copy(make_slice(array), src);
  ASSERT_EQ(memcmp(src.data, array, 3 * sizeof(int)), 0);

  int small = 0;
  copy(make_slice(&small, 1), src);
  ASSERT_EQ(small, src[0]);
}

UTEST(Slice, append_value) {
  const int values[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

  auto slice = make_slice<int>(Size(0), 1);
  defer(destroy(slice));

  for (int i = 0; i < 10; i++) {
    append(slice, values[i]);
    ASSERT_EQ(slice.len, i + 1);
    ASSERT_LE(slice.len, slice.cap);
    ASSERT_EQ(memcmp(slice.data, values, i * sizeof(int)), 0);
  }
}

UTEST(Slice, append_values) {
  const int values[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

  auto slice = make_slice<int>(1);
  defer(destroy(slice));

  append(slice, make_slice(values + 1, 9));
  ASSERT_EQ(memcmp(slice.data, values, sizeof(values)), 0);
}

UTEST(Slice, pop) {
  auto slice = make_slice<int>(10);
  defer(destroy(slice));

  const int values[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  copy(slice, make_slice(values));

  for (int i = 9; i >= 0; i--) {
    ASSERT_EQ(pop(slice), values[i]);
    ASSERT_EQ(slice.len, i);
    ASSERT_LE(slice.len, slice.cap);
  }

  EXPECT_EXCEPTION(pop(slice), int);
}

UTEST(Slice, begin_end) {
  const int array[3] = {1, 2, 3};

  auto slice = make_slice(array);
  ASSERT_EQ(begin(slice), array);
  ASSERT_EQ(end(slice), array + 3);

  Slice<int> empty = {};
  ASSERT_EQ(begin(empty), (int*)nullptr);
  ASSERT_EQ(end(empty), (int*)nullptr);
}

UTEST(Slice, bytes) {
  const int ints[3] = {};
  ASSERT_EQ(bytes(make_slice(ints)), sizeof(ints));

  const int doubles[3] = {};
  ASSERT_EQ(bytes(make_slice(doubles)), sizeof(doubles));

  Slice<int> empty = {};
  ASSERT_EQ(bytes(empty), size_t(0));
}

// -----------------------------------------------------------------------------
// MAIN
// -----------------------------------------------------------------------------

UTEST_STATE();

int main(int _argc, char** _argv) {
#if _WIN32
  auto f = freopen("NUL", "w", stderr);
#else
  auto f = freopen("/dev/null", "w", stderr);
#endif
  defer(if (f) fclose(f));

  return utest_main(_argc, _argv);
}
