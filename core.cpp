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
#  define aligned_malloc(_size, _align) ::_aligned_malloc(_size, _align)
#  define aligned_free(_ptr)            ::_aligned_free(_ptr)
#else
#  define aligned_malloc(_size, _align) ::aligned_alloc(_align, _size) // Not a typo!
#  define aligned_free(_ptr)            ::free(_ptr)
#endif

// -----------------------------------------------------------------------------
// INTERNAL HELPERS
// -----------------------------------------------------------------------------

namespace {

constexpr size_t DEFAULT_ALIGN = 2 * sizeof(void*);

constexpr Size DEFAULT_SLAB_SIZE = 8_MiB;

void copy(void* _dst, Size _dst_size, const void* _src, Size _src_size, bool _zero_mem) {
  debug_assert(_src_size >= 0);
  debug_assert(_src_size == 0 || _src);

  if (_src_size > 0) {
    memcpy(_dst, _src, size_t(_src_size));
  }

  if (_zero_mem && _dst_size > _src_size) {
    memset((u8*)_dst + _src_size, 0, size_t(_dst_size - _src_size));
  }
}

} // unnamed namespace

// -----------------------------------------------------------------------------
// DEBUGGING
// -----------------------------------------------------------------------------

void log(const char* _kind, detail::Fmt _fmt, ...) {
  va_list args;
  va_start(args, _fmt);

  char buf[sizeof(Exception::msg)];
  (void)vsnprintf(buf, sizeof(buf), _fmt.fmt, args);

  va_end(args);

  fprintf(stderr, "%s:%i: %s: %s\n", _fmt.file, _fmt.line, _kind, buf);
}

void panic(detail::Fmt _fmt, ...) {
  va_list args;
  va_start(args, _fmt);

  Exception ex = {
    .file = _fmt.file,
    .line = _fmt.line,
  };
  (void)vsnprintf(ex.msg, sizeof(ex.msg), _fmt.fmt, args);

  va_end(args);

  fprintf(stderr, "%s:%i: panic: %s\n", _fmt.file, _fmt.line, ex.msg);

#if (CORE_CONFIG_THROW_EXCEPTION_ON_PANIC)
  throw ex;
#else
  abort();
#endif
}

void panic(const char* _file, int _line, const char* _msg, ...) {
  va_list args;
  va_start(args, _msg);

  Exception ex;
  (void)vsnprintf(ex.msg, sizeof(ex.msg), _msg, args);

  va_end(args);

  fprintf(stderr, "%s:%i: panic: %s\n", _file, _line, ex.msg);

#if (CORE_CONFIG_THROW_EXCEPTION_ON_PANIC)
  throw ex;
#else
  abort();
#endif
}

namespace detail {

void panic_impl(int _line, const char* _msg, ...) {
  char buf[1024] = {};

  va_list args;
  va_start(args, _msg);
  (void)vsnprintf(buf, sizeof(buf), _msg, args);
  va_end(args);

  fprintf(stderr, "[core:%i] %s\n", _line, buf);

#if (CORE_CONFIG_THROW_EXCEPTION_ON_PANIC)
  throw _line;
#else
  abort();
#endif
}

} // namespace detail

// -----------------------------------------------------------------------------
// ALLOCATION HELPERS
// -----------------------------------------------------------------------------

void* reallocate(Allocator& _alloc, void* _ptr, Size _old, Size _new, Size _align, u8 _flags) {
  panic_if(!_alloc.alloc, "Allocator is missing the `alloc` function callback.");
  return _alloc.alloc(_alloc.ctx, _ptr, _old, _new, _align, _flags);
}

void* reallocate(void* _ptr, Size _old, Size _new, Size _align, u8 _flags) {
  return reallocate(ctx_alloc(), _ptr, _old, _new, _align, _flags);
}

void* allocate(Allocator& _alloc, Size _size, Size _align, u8 _flags) {
  return reallocate(_alloc, nullptr, 0, _size, _align, _flags & (~Allocator::FREE_ALL));
}

void* allocate(Size _size, Size _align, u8 _flags) {
  return allocate(ctx_alloc(), _size, _align, _flags);
}

void free(Allocator& _alloc, void* _ptr, Size _size) {
  (void)reallocate(_alloc, _ptr, _size, 0, DEFAULT_ALIGN);
}

void free(void* _ptr, Size _size) {
  free(ctx_alloc(), _ptr, _size);
}

void free_all(Allocator& _alloc) {
  (void)reallocate(_alloc, nullptr, 0, 0, DEFAULT_ALIGN, Allocator::FREE_ALL);
}

void free_all() {
  free_all(ctx_alloc());
}

// -----------------------------------------------------------------------------
// ALLOCATORS
// -----------------------------------------------------------------------------

Allocator std_alloc() {
  const static Allocator alloc = {
    .ctx   = nullptr,
    .alloc = [](AnyPtr&, void* _ptr, Size _old, Size _new, Size _align, u8 _flags) -> void* {
      debug_assert(_old >= 0);
      debug_assert(_new >= 0);
      debug_assert(is_power_of_two(_align));

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

Arena make_arena(ISlice<u8>&& _buf) {
  return {
    .buf  = {{_buf.data, _buf.len}},
    .head = 0,
  };
}

namespace {

void* arena_alloc(Arena& _arena, void* _ptr, Size _old, Size _new, Size _align, u8 _flags) {
  debug_assert(is_power_of_two(_align));

  if (_flags & Allocator::FREE_ALL) {
    _arena.head = 0;
    return nullptr;
  }

  if (_new <= 0) {
    return nullptr;
  }

  Size offset = align_up(_arena.buf.data + _arena.head, _align) - _arena.buf.data;
  if (offset + _new > _arena.buf.len) {
    panic_if(!(_flags & Allocator::NO_PANIC), "Failed to reallocate %td bytes aligned to a %td-byte boundary.", _new, _align);
    return nullptr;
  }

  u8* ptr     = _arena.buf.data + offset;
  _arena.head = offset + _new;

  copy(ptr, _new, _ptr, _old, !(_flags & Allocator::NON_ZERO));

  return ptr;
}

} // unnamed namespace

Allocator make_alloc(Arena& _arena) {
  return {
    .ctx   = &_arena,
    .alloc = [](AnyPtr& _ctx, void* _ptr, Size _old, Size _new, Size _align, u8 _flags) -> void* {
      debug_assert(_ctx);
      return arena_alloc(*_ctx.as<Arena>(), _ptr, _old, _new, _align, _flags);
    }};
}

// -----------------------------------------------------------------------------
// SLAB ARENA
// -----------------------------------------------------------------------------

SlabArena make_slab_arena(Allocator& _alloc, Size _slab_size) {
  SlabArena arena = {
    .slabs = make_slice<Slice<u8>>(0, 8, _alloc),
    .head  = 0,
  };

  u8* data = (u8*)allocate(_alloc, _slab_size, DEFAULT_ALIGN);
  append(arena.slabs, make_slice(data, _slab_size));

  return arena;
}

SlabArena make_slab_arena(Allocator& _alloc) {
  return make_slab_arena(_alloc, DEFAULT_SLAB_SIZE);
}

SlabArena make_slab_arena() {
  return make_slab_arena(ctx_alloc());
}

Allocator make_alloc(SlabArena& _arena) {
  return {
    .ctx   = &_arena,
    .alloc = [](AnyPtr& _ctx, void* _ptr, Size _old, Size _new, Size _align, u8 _flags) -> void* {
      debug_assert(_ctx);
      debug_assert(is_power_of_two(_align));

      SlabArena& arena = *_ctx.as<SlabArena>();

      if (_flags & Allocator::FREE_ALL) {
        for (Size i = 1; i < arena.slabs.len; i++) {
          free(*arena.slabs.alloc, arena.slabs[i].data, arena.slabs[i].len);
        }

        resize(arena.slabs, 1);
        arena.head = 0;

        return nullptr;
      }

      if (_new <= 0) {
        return nullptr;
      }

      Arena slab = {
        .buf  = arena.slabs[arena.slabs.len - 1],
        .head = arena.head,
      };

      if (void* ptr = arena_alloc(slab, _ptr, _old, _new, _align, _flags | Allocator::NO_PANIC)) {
        arena.head = slab.head;

        return ptr;
      }

      const Size size = max(_new, arena.slabs[0].len);
      if (void* ptr = reallocate(*arena.slabs.alloc, _ptr, _old, size, _align, _flags)) {
        append(arena.slabs, make_slice((u8*)ptr, size));
        arena.head = size;

        return ptr;
      }

      return nullptr;
    },
  };
}

void destroy(SlabArena& _arena) {
  for (auto slab : _arena.slabs) {
    free(*_arena.slabs.alloc, slab.data, slab.len);
  }

  destroy(_arena.slabs);
}
