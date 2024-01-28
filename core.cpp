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
#  define aligned_free(_ptr)            _aligned_free(_ptr)
#else
#  define aligned_malloc(_size, _align) aligned_alloc(_align, _size) // Not a typo!
#  define aligned_free(_ptr)            free(_ptr)
#endif

// -----------------------------------------------------------------------------
// INTERNAL HELPERS
// -----------------------------------------------------------------------------

namespace {

constexpr size_t DEFAULT_ALIGN = 2 * sizeof(void*);

constexpr Size operator""_MiB(unsigned long long _x) {
  return 1024 * 1024 * Size(_x);
}

constexpr Size DEFAULT_SLAB_SIZE = 8_MiB;

void copy(void* _dst, Size _dst_size, const void* _src, Size _src_size, bool _zero_mem) {
  assert(_src_size >= 0);
  assert(_src_size == 0 || _src);

  if (_src_size > 0) {
    memcpy(_dst, _src, size_t(_src_size));
  }

  if (_zero_mem && _dst_size > _src_size) {
    memset((u8*)_dst + _src_size, 0, size_t(_dst_size - _src_size));
  }
}

} // unnamed namespace

// -----------------------------------------------------------------------------
// PANIC
// -----------------------------------------------------------------------------

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

// -----------------------------------------------------------------------------
// ALLOCATION HELPERS
// -----------------------------------------------------------------------------

void* reallocate(const Allocator& _alloc, void* _ptr, Size _old, Size _new, Size _align, u8 _flags) {
  panic_if(!_alloc.alloc, "Allocator is missing the `alloc` function callback.");
  return _alloc.alloc(_alloc.ctx, _ptr, _old, _new, _align, _flags);
}

void* reallocate(void* _ptr, Size _old, Size _new, Size _align, u8 _flags) {
  return reallocate(ctx_alloc(), _ptr, _old, _new, _align, _flags);
}

void* allocate(const Allocator& _alloc, Size _size, Size _align, u8 _flags) {
  return reallocate(_alloc, nullptr, 0, _size, _align, _flags & (~Allocator::FREE_ALL));
}

void* allocate(Size _size, Size _align, u8 _flags) {
  return allocate(ctx_alloc(), _size, _align, _flags);
}

void free(const Allocator& _alloc, void* _ptr, Size _size) {
  (void)reallocate(_alloc, _ptr, _size, 0, DEFAULT_ALIGN);
}

void free(void* _ptr, Size _size) {
  free(ctx_alloc(), _ptr, _size);
}

void free_all(const Allocator& _alloc) {
  (void)reallocate(_alloc, nullptr, 0, 0, Allocator::FREE_ALL);
}

void free_all() {
  free_all(ctx_alloc());
}

// -----------------------------------------------------------------------------
// ALLOCATORS
// -----------------------------------------------------------------------------

const Allocator& std_alloc() {
  const static Allocator alloc = {
    .ctx   = nullptr,
    .alloc = [](void*, void* _ptr, Size _old, Size _new, Size _align, u8 _flags) -> void* {
      assert(_old >= 0);
      assert(_new >= 0);
      assert(is_power_of_two(_align));

      panic_if(_flags & Allocator::FREE_ALL, "std_alloc() doesn't support FREE_ALL mode.");

      if (_new <= 0) {
        aligned_free(_ptr);
        return nullptr;
      }

      // https://nanxiao.me/en/the-alignment-of-dynamically-allocating-memory/
      const size_t align = max(DEFAULT_ALIGN, size_t(_align));
      const size_t size  = align_up(size_t(_new), align);

      void* ptr = aligned_malloc(size, align);
      if (!ptr) {
        panic_if(!(_flags & Allocator::NO_PANIC), "Failed to reallocate %td bytes aligned to a %td-byte boundary.", _new, _align);
        return nullptr;
      }

      copy(ptr, _new, _ptr, _old, !(_flags & Allocator::NON_ZERO));

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
    alloc = make_alloc(arena);
  }
};

Allocator& ctx_temp_alloc() {
  thread_local DefaultTempAlloc alloc;

  return ctx_alloc();
}

// -----------------------------------------------------------------------------
// SIMPLE ARENA
// -----------------------------------------------------------------------------

Arena make_arena(Slice<u8>&& _buf) {
  return {
    .data = _buf.data,
    .head = 0,
    .cap  = _buf.len,
  };
}

namespace {

void* arena_alloc(Arena& _arena, void* _ptr, Size _old, Size _new, Size _align, u8 _flags) {
  assert(is_power_of_two(_align));

  if (_flags & Allocator::FREE_ALL) {
    _arena.head = 0;
    return nullptr;
  }

  if (_new <= 0) {
    return nullptr;
  }

  Size offset = align_up(_arena.data + _arena.head, _align) - _arena.data;
  if (offset + _new > _arena.cap) {
    panic_if(!(_flags & Allocator::NO_PANIC), "Failed to reallocate %td bytes aligned to a %td-byte boundary.", _new, _align);
    return nullptr;
  }

  u8* ptr     = _arena.data + offset;
  _arena.head = offset + _new;

  copy(ptr, _new, _ptr, _old, !(_flags & Allocator::NON_ZERO));

  return ptr;
}

} // unnamed namespace

Allocator make_alloc(Arena& _arena) {
  return {
    .ctx   = &_arena,
    .alloc = [](void* _ctx, void* _ptr, Size _old, Size _new, Size _align, u8 _flags) -> void* {
      assert(_ctx);
      return arena_alloc(*(Arena*)_ctx, _ptr, _old, _new, _align, _flags);
    }};
}

// -----------------------------------------------------------------------------
// SLAB ARENA
// -----------------------------------------------------------------------------

SlabArena make_slab_arena(Allocator& _alloc) {
  return {
    .slabs  = make_slice<u8*>(DEFAULT_SLAB_SIZE, _alloc),
    .active = 0,
    .head   = 0,
  };
}

SlabArena make_slab_arena() {
  return make_slab_arena(ctx_alloc());
}

Allocator make_alloc(SlabArena& _arena) {
  return {
    .ctx   = &_arena,
    .alloc = [](void* _ctx, void* _ptr, Size _old, Size _new, Size _align, u8 _flags) -> void* {
      assert(_ctx);
      assert(is_power_of_two(_align));

      SlabArena& arena = *(SlabArena*)_ctx;

      if (_flags & Allocator::FREE_ALL) {
        for (Size i = 1; i < arena.slabs.len; i++) {
          free(*arena.slabs.alloc, arena.slabs[i], DEFAULT_SLAB_SIZE);
        }

        arena.active = 0;
        arena.head   = 0;

        return nullptr;
      }

      if (_new <= 0) {
        return nullptr;
      }

      Arena slab = {
        .data = arena.slabs[arena.active],
        .head = arena.head,
        .cap  = DEFAULT_SLAB_SIZE,
      };

      if (void* ptr = arena_alloc(slab, _ptr, _old, _new, _align, _flags | Allocator::NO_PANIC)) {
        return ptr;
      }

      const Size size = max(_new, DEFAULT_SLAB_SIZE);
      if (void* ptr = reallocate(*arena.slabs.alloc, _ptr, _old, size, _align, _flags)) {
        append(arena.slabs, (u8*)ptr);
        arena.active++;
        arena.head = size;

        return ptr;
      }

      return nullptr;
    },
  };
}

void destroy(SlabArena& _arena) {
  for (u8* slab : _arena.slabs) {
    free(*_arena.slabs.alloc, slab, DEFAULT_SLAB_SIZE);
  }

  destroy(_arena.slabs);
}
