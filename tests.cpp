#include "core.h"

int main(int, char**) {
  auto vals = make_slice<int>(8);
  defer(destroy(vals));

  for (Index i = 0; i < vals.len; i++) {
    vals[i] = i;
  }

  for (int i : vals) {
    printf("%d ", i);
  }

  append(vals, 8);

  for (int i : vals) {
    printf("%d ", i);
  }

  auto slice = vals(1, 4);
  assert(slice.len == 2);

  int  rbuf[8];
  auto ring = init_ring(make_slice(rbuf));

  push(ring, 1);
  push(ring, 2);

  while (!empty(ring)) {
    printf("%i\n", pop(ring));
  }

  return vals[1] == 0;
}
