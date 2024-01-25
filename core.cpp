// MIT License
//
// Copyright (c) 2024 Jakub Fiser
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "core.h"

#include <stdarg.h> // va_*, vsnprintf
#include <stdio.h>  // fprintf, stderr
#include <stdlib.h> // abort, aligned_alloc, free

#if (_MSC_VER)
#  include <malloc.h> // _aligned_free, _aligned_malloc
#  define aligned_malloc(_size, _align) _aligned_malloc(_size, _align)
#  define aligned_free(_ptr) _aligned_free(_ptr)
#else
#  define aligned_malloc(_size, _align) aligned_alloc(_align,  _size) // Not a typo!
#  define aligned_free(_ptr) free(_ptr)
#endif

namespace {

constexpr size_t DEFAULT_ALIGN = 2 * sizeof(void*);

constexpr Size operator""_MiB(unsigned long long _x) {
  return 1024 * 1024 * Size(_x);
}

void copy_and_zero(void* _dst, Size _dst_size, const void* _src, Size _src_size) {
  assert(_src_size >= 0);
  assert(_src_size == 0 || _src);

  if (_src_size > 0) {
    memcpy(_dst, _src, size_t(_src_size));
  }

  if (_dst_size > _src_size) {
    memset((u8*)_dst + _src_size, 0, size_t(_dst_size - _src_size));
  }
}

} // anonymous namespace

namespace detail {

void panic_impl(int _line, const char* _msg, ...) {
  char buf[1024] = {};

  va_list args;
  va_start(args, _msg);
  (void)vsnprintf(buf, sizeof(buf), _msg, args);
  va_end(args);

  fprintf(stderr, "[core:%i] %s\n", _line, buf);

#if defined(THROW_EXCEPTION_ON_PANIC)
  throw _line;
#else
  abort();
#endif
}

} // namespace detail

void* allocate(const Allocator& _alloc, void* _ptr, Size _old, Size _new, Size _align) {
  panic_if(!_alloc.alloc, "Allocator is missing the `alloc` function callback.");
  return _alloc.alloc(_alloc.ctx, _ptr, _old, _new, _align);
}

const Allocator& std_alloc() {
  const static Allocator alloc = {
    .ctx   = nullptr,
    .alloc = [](void*, void* _ptr, Size _old, Size _new, Size _align) -> void* {
      assert(_old >= 0);
      assert(_new >= 0);
      assert(is_power_of_two(_align));

      if (_new <= 0) {
        aligned_free(_ptr);
        return nullptr;
      }

      // https://nanxiao.me/en/the-alignment-of-dynamically-allocating-memory/
      const size_t align = max(DEFAULT_ALIGN, size_t(_align));
      const size_t size  = align_up(size_t(_new), align);

      void* ptr = aligned_malloc(size, align);
      panic_if(!ptr, "Failed to allocate %td bytes aligned to a %td-byte boundary.", _new, _align);

      copy_and_zero(ptr, _new, _ptr, _old);

      if (_ptr) {
        aligned_free(_ptr);
      }

      return ptr;
    },
  };

  return alloc;
}

Allocator& ctx_alloc() {
  thread_local Allocator alloc = std_alloc();
  return alloc;
}

struct DefaultTempAlloc {
  Allocator alloc;
  Arena     arena;
  u8        buf[4_MiB];

  DefaultTempAlloc() {
    arena = make_arena(make_slice(buf));
    alloc = make_arena_alloc(arena);
  }
};

Allocator& ctx_temp_alloc() {
  thread_local DefaultTempAlloc alloc;

  return ctx_alloc();
}

Arena make_arena(Slice<u8>&& _buf) {
  return {
    .first = begin(_buf),
    .last  = end(_buf),
  };
}

Allocator make_arena_alloc(Arena& _arena) {
  return {
    .ctx   = &_arena,
    .alloc = [](void* _ctx, void* _ptr, Size _old, Size _new, Size _align) -> void* {
      assert(_ctx);
      assert(is_power_of_two(_align));

      if (_new <= 0) {
        return nullptr;
      }

      Arena& arena = *(Arena*)_ctx;
      u8*    ptr   = align_up(arena.first, _align);

      // TODO : Runtime-defined behavior instead?
      panic_if(ptr + _new > arena.last, "Failed to allocate %td bytes aligned to a %td-byte boundary.", _new, _align);

      arena.first = ptr + _new;
      copy_and_zero(ptr, _new, _ptr, _old);

      return ptr;
    }};
}
