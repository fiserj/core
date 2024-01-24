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

#pragma once

#include <stddef.h> // ptrdiff_t, size_t
#include <stdint.h> // uint8_t
#include <stdio.h>  // fprintf, stderr
#include <string.h> // memcpy, memset

// -----------------------------------------------------------------------------
// UTILITY MACROS
// -----------------------------------------------------------------------------

#define CONCAT_(_x, _y) _x##_y
#define CONCAT(_x, _y) CONCAT_(_x, _y)

// -----------------------------------------------------------------------------
// DEBUGGING
// -----------------------------------------------------------------------------

#if (_MSC_VER)
#  define debug_break __debugbreak
#elif (__has_builtin(__builtin_debugtrap))
#  define debug_break __builtin_debugtrap
#elif (__has_builtin(__builtin_trap))
#  define debug_break __builtin_trap
#else
#  define debug_break abort
#endif

#define assert(_cond) \
  if (!(_cond))       \
  debug_break()

namespace detail {

void panic_impl([[maybe_unused]] int _line);

} // namespace detail

#define panic(_msg, ...)                     \
  do {                                       \
    fprintf(stderr, _msg "\n", __VA_ARGS__); \
    detail::panic_impl(__LINE__);            \
  } while (0)

#define panic_if(_cond, _msg, ...) \
  if (_cond)                       \
  panic(_msg, __VA_ARGS__)

#if (NO_BOUNDS_CHECK)
#  define check_bounds(_cond) ((void)0)
#else
#  define check_bounds(_cond) \
    panic_if(!(_cond), "Out of bounds: %s", #_cond)
#endif

// -----------------------------------------------------------------------------
// SIZED TYPES' ALIASES
// -----------------------------------------------------------------------------

using Index = ptrdiff_t;
using Size  = Index;

using i8  = uint8_t;
using i16 = uint16_t;
using i32 = uint32_t;
using i64 = uint64_t;

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using f32 = float;
using f64 = double;

// -----------------------------------------------------------------------------
// TAGS
// -----------------------------------------------------------------------------

struct OmittedTag {};

constexpr OmittedTag _;

// -----------------------------------------------------------------------------
// NON COPYABLE BASE
// -----------------------------------------------------------------------------

struct NonCopyable {
  NonCopyable() = default;

  NonCopyable(const NonCopyable&) = delete;

  NonCopyable& operator=(const NonCopyable&) = delete;
};

// -----------------------------------------------------------------------------
// DEFERRED EXECUTION
// -----------------------------------------------------------------------------

namespace detail {

template <typename F>
struct Deferred : NonCopyable {
  F func;

  Deferred(F&& _func)
    : func(static_cast<F&&>(_func)) {}

  ~Deferred() { func(); }
};

} // namespace detail

#define defer(...) \
  ::detail::Deferred CONCAT(deferred_, __LINE__)([&]() mutable { __VA_ARGS__; })

// -----------------------------------------------------------------------------
// SMALL UTILITIES
// -----------------------------------------------------------------------------

template <typename T>
constexpr T min(T _x, T _y) {
  return _x < _y ? _x : _y;
}

template <typename T>
constexpr T max(T _x, T _y) {
  return _x > _y ? _x : _y;
}

template <typename T>
constexpr T clamp(T _x, T _min, T _max) {
  return min(max(_x, _min), _max);
}

template <typename T>
constexpr bool is_power_of_two(T _value) {
  return (_value & (_value - 1)) == 0;
}

template <typename T>
constexpr T align_up(T _value, T _align) {
  return (_value + _align - 1) & ~(_align - 1);
}

// -----------------------------------------------------------------------------
// ALLOCATORS
// -----------------------------------------------------------------------------

struct Allocator {
  void* ctx;

  void* (*alloc)(void* _ctx, void* _ptr, Size _old, Size _new, Size _align);
};

const Allocator& std_alloc();

Allocator& ctx_alloc();

Allocator& ctx_temp_alloc();

void* allocate(const Allocator& _alloc, void* _ptr, Size _old, Size _new, Size _align);

namespace detail {

struct ScopedAllocator : NonCopyable {
  Allocator prev;

  ScopedAllocator(const Allocator& _alloc)
    : prev(ctx_alloc()) { ctx_alloc() = _alloc; }

  ~ScopedAllocator() { ctx_alloc() = prev; }
};

} // namespace detail

#define scope_alloc(_alloc) \
  ::detail::ScopedAllocator CONCAT(scoped_alloc, __LINE__)(_alloc)

// -----------------------------------------------------------------------------
// SLICE
// -----------------------------------------------------------------------------

namespace detail {

template <typename T>
struct Slice {
  T*   data;
  Size len;

  operator bool() const {
    return data != nullptr;
  }

  T& operator[](Index _i) const {
    check_bounds(_i >= 0 && _i < len);
    return data[_i];
  }

  Slice<T> operator()(Index _low, Index _high) const {
    check_bounds(_low >= 0);
    check_bounds(_low <= _high);
    check_bounds(_high <= len);
    return {
      .data = data + _low,
      .len  = _high - _low,
    };
  }

  Slice<T> operator()(OmittedTag, Index _high) const {
    return operator()(0, _high);
  }

  Slice<T> operator()(Index _low, OmittedTag) const {
    return operator()(_low, len);
  }
};

constexpr Size next_cap(Size _cap, Size _req) {
  return max(max(Size(8), _req), (_cap * 3) / 2);
}

} // namespace detail

constexpr bool Dynamic = true;

template <typename T, bool IsDynamic = false>
struct Slice : detail::Slice<T> {};

template <typename T>
struct Slice<T, Dynamic> : detail::Slice<T> {
  Size       cap;
  Allocator* alloc;
};

template <typename T>
using DSlice = Slice<T, Dynamic>;

template <typename T>
Slice<T, Dynamic> make_slice(Size _len, Size _cap, Allocator& _alloc) {
  assert(_len >= 0 && _len <= _cap);
  assert(_alloc.alloc);

  Slice<T, Dynamic> slice;
  slice.data  = (T*)_alloc.alloc(_alloc.ctx, nullptr, 0, _cap * sizeof(T), alignof(T));
  slice.len   = _len;
  slice.cap   = _cap;
  slice.alloc = &_alloc;

  return slice;
}

template <typename T>
Slice<T, Dynamic> make_slice(Size _len, Size _cap) {
  return make_slice<T>(_len, _cap, ctx_alloc());
}

template <typename T>
Slice<T, Dynamic> make_slice(Size _len) {
  return make_slice<T>(_len, _len);
}

template <typename T>
Slice<T, Dynamic> make_slice(Size _len, Allocator& _alloc) {
  return make_slice<T>(_len, _len, _alloc);
}

template <typename T, Size N>
Slice<T> make_slice(T (&_array)[N]) {
  Slice<T> slice;
  slice.data = _array;
  slice.len  = N;

  return slice;
}

template <typename T>
Slice<T> make_slice(T* _data, Size _len) {
  Slice<T> slice;
  slice.data = _data;
  slice.len  = _len;

  return slice;
}

template <typename T>
void destroy(Slice<T, Dynamic>& _slice) {
  assert(_slice.alloc);
  _slice.alloc->alloc(_slice.alloc->ctx, _slice.data, size_t(_slice.cap) * sizeof(T), 0, alignof(T));
}

template <typename T>
void reserve(Slice<T, Dynamic>& _slice, Size _cap) {
  if (_cap <= _slice.cap) {
    return;
  }

  _slice.data = (T*)_slice.alloc->alloc(_slice.alloc->ctx, _slice.data, size_t(_slice.cap) * sizeof(T), _cap * sizeof(T), alignof(T));
  _slice.cap  = _cap;
}

template <typename T>
void resize(Slice<T, Dynamic>& _slice, Size _len) {
  assert(_len >= 0);

  if (_len <= _slice.len) {
    _slice.len = _len;
    return;
  }

  if (_len <= _slice.cap) {
    memset(_slice.data + _slice.len, 0, size_t(_len - _slice.len) * sizeof(T));
    _slice.len = _len;
    return;
  }

  reserve(_slice, detail::next_cap(_slice.cap, _len));
  _slice.len = _len;
}

template <typename T>
void copy(detail::Slice<T>& _dst, const detail::Slice<T>& _src) {
  memcpy(_dst.data, _src.data, size_t(min(_dst.len, _src.len)) * sizeof(T));
}

template <typename T>
void append(Slice<T, Dynamic>& _slice, const T& _value) {
  if (_slice.len == _slice.cap) {
    reserve(_slice, detail::next_cap(_slice.cap, _slice.len + 1));
  }

  _slice.data[_slice.len++] = _value;
}

template <typename T>
void append(Slice<T, Dynamic>& _slice, const detail::Slice<T>& _values) {
  const Size low = _slice.len;
  const Size len = _slice.len + _values.len;

  if (len >= _slice.cap) {
    reserve(_slice, detail::next_cap(_slice.cap, len));
  }

  copy(_slice(low, _), _values);
}

template <typename T>
T& pop(Slice<T, Dynamic>& _slice) {
  assert(_slice.len > 0);
  return _slice.data[--_slice.len];
}

template <typename T>
T* begin(const detail::Slice<T>& _slice) {
  return _slice.data;
}

template <typename T>
T* end(const detail::Slice<T>& _slice) {
  return _slice.data + _slice.len;
}

template <typename T>
size_t bytes(const detail::Slice<T>& _slice) {
  return size_t(_slice.len) * sizeof(T);
}

// -----------------------------------------------------------------------------
// ARENA
// -----------------------------------------------------------------------------

struct Arena {
  u8* first;
  u8* last;
};

Arena make_arena(Slice<u8>&& _buf);

Allocator make_arena_alloc(Arena& _arena);

// -----------------------------------------------------------------------------
// RING BUFFER
// -----------------------------------------------------------------------------

template <typename T>
struct Ring {
  Slice<T> buf;
  Size     head;
  Size     tail;
};

template <typename T>
Ring<T> init_ring(detail::Slice<T>&& _buf) {
  Ring<T> ring;
  ring.buf  = {{_buf.data, _buf.len}};
  ring.head = 0;
  ring.tail = 0;

  return ring;
}

template <typename T>
void push(Ring<T>& _ring, const T& _value) {
  const Size head = (_ring.head + 1) % _ring.buf.len;
  check_bounds(head != _ring.tail);

  _ring.buf[_ring.head] = _value;
  _ring.head            = head;
}

template <typename T>
T& pop(Ring<T>& _ring) {
  check_bounds(_ring.head != _ring.tail);

  T& elem    = _ring.buf[_ring.tail];
  _ring.tail = (_ring.tail + 1) % _ring.buf.len;

  return elem;
}

template <typename T>
bool empty(const Ring<T>& _ring) {
  return _ring.head == _ring.tail;
}
