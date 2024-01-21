#include <stdio.h> // freopen, stderr

#include <utest.h>

#include "core.h"

UTEST(defer, order) {
  int val = 1;

  defer(ASSERT_EQ(val++, 3));
  defer(ASSERT_EQ(val++, 2));
  defer(ASSERT_EQ(val++, 1));
}

UTEST(make_slice, array) {
  int array[3] = {1, 2, 3};

  auto slice = make_slice(array);

  ASSERT_EQ(slice.data, array);
  ASSERT_EQ(slice.len, 3);

  ASSERT_EQ(slice[0], 1);
  ASSERT_EQ(slice[1], 2);
  ASSERT_EQ(slice[2], 3);

  EXPECT_EXCEPTION(slice[-1], int);
  EXPECT_EXCEPTION(slice[+4], int);
}

UTEST_STATE();

int main(int _argc, char** _argv) {
  freopen("/dev/null", "w", stderr);

  return utest_main(_argc, _argv);
}
