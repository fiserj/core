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
#include <stdint.h> // *int*_t, uintptr_t
#include <string.h> // memcpy, memset

// -----------------------------------------------------------------------------
// CONFIG
// -----------------------------------------------------------------------------

#if !defined(CORE_CONFIG_DEBUG_MODE)
#  if defined(NDEBUG)
#    define CORE_CONFIG_DEBUG_MODE 0
#  else
#    define CORE_CONFIG_DEBUG_MODE 1
#  endif
#endif

#if !defined(CORE_CONFIG_NO_BOUNDS_CHECK)
#  define CORE_CONFIG_NO_BOUNDS_CHECK 0
#endif

#if !defined(CORE_CONFIG_THROW_EXCEPTION_ON_PANIC)
#  define CORE_CONFIG_THROW_EXCEPTION_ON_PANIC 0
#endif

// -----------------------------------------------------------------------------
// DEBUGGING
// -----------------------------------------------------------------------------

#if (CORE_CONFIG_DEBUG_MODE)
#  define debug_assert(_cond)    \
    do {                         \
      if (!(_cond)) {            \
        ::log("assert", #_cond); \
        ::debug_break();         \
      }                          \
    } while (0)
#else
#  define debug_assert(_cond) ((void)0)
#endif

#define panic_if(_cond, ...) \
  do {                       \
    if (_cond)               \
      ::panic(__VA_ARGS__);  \
  } while (0)

inline void debug_break() {
#if (_MSC_VER)
  __debugbreak();
#elif (__has_builtin(__builtin_debugtrap))
  __builtin_debugtrap();
#elif (__has_builtin(__builtin_trap))
  __builtin_trap();
#else
  __asm__("int3");
#endif
}

#if (CORE_CONFIG_NO_BOUNDS_CHECK)
#  define check_bounds(_cond) ((void)0)
#else
#  define check_bounds(_cond) \
    panic_if(!(_cond), "Bounds check failure: %s", #_cond)
#endif

struct Exception {
  const char* file;
  int         line;
  char        msg[256];
};

namespace detail {

struct Fmt {
  const char* fmt;
  const char* file;
  int         line;

  constexpr Fmt(const char* _fmt, const char* _file = __builtin_FILE(), int _line = __builtin_LINE())
    : fmt(_fmt), file(_file), line(_line) {}
};

} // namespace detail

void log(const char* _kind, detail::Fmt _fmt, ...);

[[noreturn]] void panic(detail::Fmt _fmt, ...);

// -----------------------------------------------------------------------------
// SIZED TYPES' ALIASES
// -----------------------------------------------------------------------------

using Index = ptrdiff_t;
using Size  = Index;

using i8  = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

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
// TYPE TRAITS
// -----------------------------------------------------------------------------

namespace detail {

template <typename T>
struct RemoveConst {
  using Type = T;
};
template <typename T>
struct RemoveConst<const T> {
  using Type = T;
};

template <typename T, typename U>
struct TypeEquivalence {
  static constexpr bool val = false;
};

template <typename T>
struct TypeEquivalence<T, T> {
  static constexpr bool val = true;
};

template <typename T>
struct TypeId {
  static constexpr int val = 0;
};

template <typename T>
constexpr uintptr_t type_id() {
  return uintptr_t(&detail::TypeId<T>::val);
}

template <>
constexpr uintptr_t type_id<decltype(nullptr)>() {
  return uintptr_t(0);
}

} // namespace detail

template <typename T>
using NonConst = typename detail::RemoveConst<T>::Type;

template <typename T>
using Const = const typename detail::RemoveConst<T>::Type;

template <typename T, typename U>
constexpr bool is_same = detail::TypeEquivalence<T, U>::val;

template <typename T>
uintptr_t TypeId = detail::type_id<T>();

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

#define CONCAT_(_x, _y) _x##_y

#define CONCAT(_x, _y) CONCAT_(_x, _y)

#define defer(...) \
  ::detail::Deferred CONCAT(deferred_, __LINE__)([&]() mutable { __VA_ARGS__; })

// -----------------------------------------------------------------------------
// SMALL UTILITIES
// -----------------------------------------------------------------------------

constexpr Size operator""_KiB(unsigned long long _x) {
  return 1024 * Size(_x);
}

constexpr Size operator""_MiB(unsigned long long _x) {
  return 1024 * 1024 * Size(_x);
}

constexpr Size operator""_GiB(unsigned long long _x) {
  return 1024 * 1024 * 1024 * Size(_x);
}

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

template <typename T, typename A>
constexpr T align_up(T _value, A _align) {
  return (_value + _align - 1) & ~(_align - 1);
}

template <typename T, typename A>
constexpr T* align_up(T* _ptr, A _align) {
  union Conv {
    T*        ptr;
    uintptr_t addr;
  };

  Conv conv;
  conv.ptr  = _ptr;
  conv.addr = align_up(conv.addr, _align);

  return conv.ptr;
}

// -----------------------------------------------------------------------------
// ANY PTR
// -----------------------------------------------------------------------------

struct AnyPtr {
  const void* ptr  = nullptr;
  uintptr_t   type = 0;

  AnyPtr() = default;

  AnyPtr(decltype(nullptr))
    : ptr(nullptr), type(0) {}

  template <typename T>
  AnyPtr(T* _ptr)
    : ptr(_ptr), type(TypeId<T>) {}

  operator bool() const {
    return ptr != nullptr;
  }

  template <typename T>
  T* as() const {
    if (type != TypeId<T> && type != TypeId<NonConst<T>>) {
      panic("Failed to safely type-cast AnyPtr.");
      return nullptr;
    }

    return static_cast<T*>(const_cast<void*>(ptr));
  }
};

// -----------------------------------------------------------------------------
// ALLOCATORS
// -----------------------------------------------------------------------------

struct Allocator {
  enum Flags : u8 {
    DEFAULT  = 0x0,
    FREE_ALL = 0x1,
    NON_ZERO = 0x2,
    NO_PANIC = 0x4,
  };

  AnyPtr ctx;

  void* (*alloc)(AnyPtr& _ctx, void* _ptr, Size _old, Size _new, Size _align, u8 _flags);
};

Allocator std_alloc();

Allocator& ctx_alloc();

Allocator& ctx_temp_alloc();

namespace detail {

struct ScopedAllocator : NonCopyable {
  Allocator prev;

  ScopedAllocator(const Allocator& _alloc)
    : prev(ctx_alloc()) { ctx_alloc() = _alloc; }

  ~ScopedAllocator() { ctx_alloc() = prev; }
};

} // namespace detail

#define scope_alloc(_alloc) \
  ::detail::ScopedAllocator CONCAT(scoped_alloc_, __LINE__)(_alloc)

// -----------------------------------------------------------------------------
// ALLOCATION HELPERS
// -----------------------------------------------------------------------------

void* reallocate(Allocator& _alloc, void* _ptr, Size _old, Size _new, Size _align, u8 _flags = Allocator::DEFAULT);

void* reallocate(void* _ptr, Size _old, Size _new, Size _align, u8 _flags = Allocator::DEFAULT);

void* allocate(Allocator& _alloc, Size _size, Size _align, u8 _flags = Allocator::DEFAULT);

void* allocate(Size _size, Size _align, u8 _flags = Allocator::DEFAULT);

void free(Allocator& _alloc, void* _ptr, Size _size);

void free(void* _ptr, Size _size);

void free_all(Allocator& _alloc);

void free_all();

// -----------------------------------------------------------------------------
// SLICE
// -----------------------------------------------------------------------------

template <typename T>
struct ISlice {
  T*   data;
  Size len;

  operator bool() const {
    return data != nullptr;
  }

  operator ISlice<Const<T>>() const {
    return {
      .data = data,
      .len  = len,
    };
  }

  T& operator[](Index _i) const {
    check_bounds(_i >= 0 && _i < len);
    return data[_i];
  }

  ISlice<T> operator()(Index _low, Index _high) const {
    check_bounds(_low >= 0);
    check_bounds(_low <= _high);
    check_bounds(_high <= len);
    return {
      .data = data + _low,
      .len  = _high - _low,
    };
  }

  ISlice<T> operator()(OmittedTag, Index _high) const {
    return operator()(0, _high);
  }

  ISlice<T> operator()(Index _low, OmittedTag) const {
    return operator()(_low, len);
  }
};

namespace detail {

constexpr Size next_cap(Size _cap, Size _req) {
  return max(max(Size(8), _req), (_cap * 3) / 2);
}

} // namespace detail

constexpr bool Dynamic = true;

template <typename T, bool IsDynamic = false>
struct Slice : ISlice<T> {};

template <typename T>
struct Slice<T, Dynamic> : ISlice<T> {
  Size       cap;
  Allocator* alloc;
};

template <typename T>
Slice<T, Dynamic> make_slice(Size _len, Size _cap, Allocator& _alloc) {
  debug_assert(_len >= 0 && _len <= _cap);
  debug_assert(_alloc.alloc);

  // TODO : Can we make this work with designated initializers?
  return {
    {(T*)allocate(_alloc, _cap * sizeof(T), alignof(T)),
     _len},
    _cap,
    &_alloc,
  };
}

template <typename T, typename S>
Slice<T, Dynamic> make_slice(S _len, Size _cap) {
  static_assert(is_same<S, Size> || is_same<S, decltype(0)>);

  return make_slice<T>(Size(_len), _cap, ctx_alloc());
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
Slice<T> make_slice(decltype(nullptr), Size) = delete;

template <typename T>
void destroy(Slice<T, Dynamic>& _slice) {
  free(*_slice.alloc, _slice.data, _slice.cap * sizeof(T));
}

template <typename T>
void reserve(Slice<T, Dynamic>& _slice, Size _cap) {
  if (_cap <= _slice.cap) {
    return;
  }

  _slice.data = (T*)reallocate(*_slice.alloc, _slice.data, _slice.cap * sizeof(T), _cap * sizeof(T), alignof(T));
  _slice.cap  = _cap;
}

template <typename T>
void resize(Slice<T, Dynamic>& _slice, Size _len) {
  debug_assert(_len >= 0);

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
void copy(const ISlice<T>& _dst, const ISlice<Const<T>>& _src) {
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
void append(Slice<T, Dynamic>& _slice, const ISlice<Const<T>>& _values) {
  const Size low = _slice.len;
  const Size len = _slice.len + _values.len;

  if (len >= _slice.cap) {
    reserve(_slice, detail::next_cap(_slice.cap, len));
  }

  _slice.len = len;
  copy(_slice(low, len), _values);
}

template <typename T>
T& pop(Slice<T, Dynamic>& _slice) {
  check_bounds(_slice.len > 0);
  return _slice.data[--_slice.len];
}

template <typename T>
bool empty(const ISlice<T>& _slice) {
  debug_assert(_slice.len >= 0);
  return _slice.len == 0;
}

template <typename T>
T* begin(const ISlice<T>& _slice) {
  return _slice.data;
}

template <typename T>
T* end(const ISlice<T>& _slice) {
  return _slice.data + _slice.len;
}

template <typename T>
size_t bytes(const ISlice<T>& _slice) {
  return size_t(_slice.len) * sizeof(T);
}

// -----------------------------------------------------------------------------
// SIMPLE ARENA (NON-OWNING)
// -----------------------------------------------------------------------------

struct Arena {
  Slice<u8> buf;
  Index     head;
};

Arena make_arena(ISlice<u8>&& _buf);

Allocator make_alloc(Arena& _arena);

// -----------------------------------------------------------------------------
// SLAB ARENA (GROWABLE)
// -----------------------------------------------------------------------------

struct SlabArena {
  Slice<Slice<u8>, Dynamic> slabs;
  Index                     head;
};

SlabArena make_slab_arena(Allocator& _alloc);

SlabArena make_slab_arena();

Allocator make_alloc(SlabArena& _arena);

void destroy(SlabArena& _arena);

// -----------------------------------------------------------------------------
// RING BUFFER
// -----------------------------------------------------------------------------

template <typename T>
struct Ring {
  Slice<T> buf;
  Index    head;
  Index    tail;
};

template <typename T>
Ring<T> make_ring(ISlice<T>&& _buf) {
  debug_assert(_buf.len > 1);

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
