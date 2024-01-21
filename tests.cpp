#include <stdio.h> // freopen, stderr

#include <utest.h>

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

// -----------------------------------------------------------------------------
// ALLOCATORS
// -----------------------------------------------------------------------------

// ...

// -----------------------------------------------------------------------------
// SLICE
// -----------------------------------------------------------------------------

UTEST(Slice, subscript_operator) {
  int array[3] = {1, 2, 3};

  detail::Slice<int> slice = {
    .data = array,
    .len  = sizeof(array) / sizeof(array[0]),
  };

  ASSERT_EQ(slice[0], 1);
  ASSERT_EQ(slice[1], 2);
  ASSERT_EQ(slice[2], 3);

  EXPECT_EXCEPTION(slice[-1], int);
  EXPECT_EXCEPTION(slice[+4], int);
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

UTEST_STATE();

int main(int _argc, char** _argv) {
  freopen("/dev/null", "w", stderr);

  return utest_main(_argc, _argv);
}
