// MIT License

// Copyright (c) 2024 Jakub Fiser

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <stdio.h> // freopen, stderr

#include <utest.h>

#include "core.h"

UTEST(defer, order) {
  int val = 1;

  defer(ASSERT_EQ(val++, 3));
  defer(ASSERT_EQ(val++, 2));
  defer(ASSERT_EQ(val++, 1));
}

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

  ASSERT_EQ(slice[0], 0);
  ASSERT_EQ(slice[1], 0);
  ASSERT_EQ(slice[2], 0);

  ASSERT_NE(slice.data, (int*)nullptr);
  ASSERT_EQ(slice.len, 3);
}

UTEST_STATE();

int main(int _argc, char** _argv) {
  freopen("/dev/null", "w", stderr);

  return utest_main(_argc, _argv);
}
