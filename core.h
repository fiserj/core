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

#include <math.h>   // cosf, sinf, sqrtf
#include <stddef.h> // ptrdiff_t, size_t
#include <stdint.h> // *int*_t, uintptr_t
#include <string.h> // memcpy, memmove, memset

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

#define warn(...) \
  ::log("warn", __VA_ARGS__)

#define warn_if(_cond, ...) \
  do {                      \
    if (_cond)              \
      warn(__VA_ARGS__);    \
  } while (0)

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

using b8  = i8;
using b32 = i32;

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

template <typename T, Size N>
constexpr Size countof(const T (&)[N]) noexcept {
  return N;
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
constexpr void swap(T& _x, T& _y) {
  T tmp = _x;
  _x    = _y;
  _y    = tmp;
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
    panic_if(type != TypeId<T> && type != TypeId<NonConst<T>>, "Failed to safely type-cast AnyPtr.");
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

namespace detail {

struct BackIndex {
  Index val;
};

} // namespace detail

constexpr detail::BackIndex len = {};

constexpr detail::BackIndex operator-(detail::BackIndex _i, Index _j) {
  return {_i.val - _j};
}

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

  T& operator[](detail::BackIndex _i) const {
    return operator[](len + _i.val);
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
using DynSlice = Slice<T, Dynamic>;

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
  const Size low  = _slice.len;
  const Size high = _slice.len + _values.len;

  if (high >= _slice.cap) {
    reserve(_slice, detail::next_cap(_slice.cap, high));
  }

  _slice.len = high;
  copy(_slice(low, high), _values);
}

template <typename T>
T& pop(Slice<T, Dynamic>& _slice) {
  check_bounds(_slice.len > 0);
  return _slice.data[--_slice.len];
}

template <typename T>
void remove_ordered(Slice<T, Dynamic>& _slice, Index _i) {
  check_bounds(_i >= 0 && _i < _slice.len);

  if (_i != _slice.len - 1) {
    memmove(_slice.data + _i, _slice.data + _i + 1, size_t(_slice.len - _i - 1) * sizeof(T));
  }

  _slice.len--;
}

template <typename T>
void remove_unordered(Slice<T, Dynamic>& _slice, Index _i) {
  check_bounds(_i >= 0 && _i < _slice.len);

  if (_i != _slice.len - 1) {
    _slice.data[_i] = _slice.data[_slice.len - 1];
  }

  _slice.len--;
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

Arena make_arena(ISlice<u8> _buf);

Allocator make_alloc(Arena& _arena);

// -----------------------------------------------------------------------------
// SLAB ARENA (GROWABLE)
// -----------------------------------------------------------------------------

struct SlabArena {
  Slice<Slice<u8>, Dynamic> slabs;
  Index                     head;
};

SlabArena make_slab_arena(Allocator& _alloc, Size _slab_size);

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
Ring<T> make_ring(ISlice<T> _buf) {
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

// -----------------------------------------------------------------------------
// FILE I/O
// -----------------------------------------------------------------------------

Slice<u8, Dynamic> read_bytes(const char* _path, Allocator& _alloc);

Slice<u8, Dynamic> read_bytes(const char* _path);

Slice<char, Dynamic> read_string(const char* _path, Allocator& _alloc);

Slice<char, Dynamic> read_string(const char* _path);

// -----------------------------------------------------------------------------
// 2D VECTOR
// -----------------------------------------------------------------------------

/// 2D vector of single-precision floating-point numbers.
///
struct Vec2 {
  f32 x;
  f32 y;
};

/// Negates (flips) a vector.
///
/// @param[in] _u Vector.
///
/// @return Negated vector.
///
constexpr Vec2 operator-(Vec2 _u) {
  return {-_u.x, -_u.y};
}

/// Adds two vectors.
///
/// @param[in] _u First vector.
/// @param[in] _v Second vector.
///
/// @return Sum of the two vectors.
///
constexpr Vec2 operator+(Vec2 _u, Vec2 _v) {
  return {_u.x + _v.x, _u.y + _v.y};
}

/// Subtracts two vectors.
///
/// @param[in] _u First vector.
/// @param[in] _v Second vector.
///
/// @return Difference of the two vectors.
///
constexpr Vec2 operator-(Vec2 _u, Vec2 _v) {
  return {_u.x - _v.x, _u.y - _v.y};
}

/// Multiplies a vector by a scalar.
///
/// @param[in] _u Vector.
/// @param[in] _s Scalar.
///
/// @return Scaled vector.
///
constexpr Vec2 operator*(Vec2 _u, f32 _s) {
  return {_u.x * _s, _u.y * _s};
}

/// Computes vector's squared length.
///
/// @param[in] _v Vector.
///
/// @return Squared length of the vector.
///
constexpr f32 length2(Vec2 _v) {
  return _v.x * _v.x + _v.y * _v.y;
}

/// Computes vector's length.
///
/// @param[in] _v Vector.
///
/// @return Length of the vector.
///
inline f32 length(Vec2 _v) {
  return sqrtf(length2(_v));
}

/// Returns vector's minimum component.
///
/// @param[in] _v Vector.
///
/// @return Minimum component of the vector.
///
constexpr f32 min(Vec2 _v) {
  return min(_v.x, _v.y);
}

/// Returns vector with minimum components.
///
/// @param _u First vector.
/// @param _v Second vector.
///
/// @return Vector with minimum components.
///
constexpr Vec2 min(Vec2 _u, Vec2 _v) {
  return {min(_u.x, _v.x), min(_u.y, _v.y)};
}

/// Returns vector's maximum component.
///
/// @param[in] _v Vector.
///
/// @return Maximum component of the vector.
///
constexpr f32 max(Vec2 _v) {
  return max(_v.x, _v.y);
}

/// Returns vector with maximum components.
///
/// @param _u First vector.
/// @param _v Second vector.
///
/// @return Vector with maximum components.
///
constexpr Vec2 max(Vec2 _u, Vec2 _v) {
  return {max(_u.x, _v.x), max(_u.y, _v.y)};
}

/// Returns a normalized copy of the vector (with unit length).
///
/// @param[in] _v Vector.
///
/// @return Normalized vector.
///
inline Vec2 normalized(Vec2 _v) {
  return _v * (1.0f / length(_v));
}

/// Returns a copy of the vector rotated by 90 degrees clockwise.
///
/// @param[in] _v Vector.
///
/// @return Vector rotated by 90 degrees clockwise.
///
constexpr Vec2 perpendicular_cw(Vec2 _v) {
  return {_v.y, -_v.x};
}

/// Returns a copy of the vector rotated by 90 degrees counterclockwise.
///
/// @param[in] _v Vector.
///
/// @return Vector rotated by 90 degrees counterclockwise.
///
constexpr Vec2 perpendicular_ccw(Vec2 _v) {
  return {-_v.y, _v.x};
}

/// Computes dot product of two vectors.
///
/// @param[in] _u First vector.
/// @param[in] _v Second vector.
///
/// @return Dot product of the two vectors.
///
constexpr f32 dot(Vec2 _u, Vec2 _v) {
  return _u.x * _v.x + _u.y * _v.y;
}

// -----------------------------------------------------------------------------
// 1D RANGE
// -----------------------------------------------------------------------------

/// 1D range of single-precision floating-point numbers.
struct Range {
  f32 min;
  f32 max;
};

/// Checks if two ranges overlap. Assumes that each range's minimum is less than
/// or equal to its maximum.
///
/// @param[in] _a First range.
/// @param[in] _b Second range.
///
/// @return `true` if the ranges overlap, `false` otherwise.
///
constexpr bool overlap(Range _a, Range _b) {
  return (_a.min <= _b.max) && (_b.min <= _a.max);
}

/// Constructs a range from two numbers, assignig the smaller number to `min`
/// and the larger number to `max`.
///
/// @param[in] _a First number.
/// @param[in] _b Second number.
///
/// @return Range of numbers.
///
constexpr Range min_max(f32 _a, f32 _b) {
  return _a < _b ? Range{_a, _b} : Range{_b, _a};
}

/// Constructs a range from four numbers, assignig the smallest number to `min`
/// and the largest number to `max`.
///
/// @param[in] _a First number.
/// @param[in] _b Second number.
/// @param[in] _c Third number.
/// @param[in] _d Fourth number.
///
/// @return Range of numbers.
///
constexpr Range min_max(f32 _a, f32 _b, f32 _c, f32 _d) {
  auto r = min_max(_a, _b);

  // clang-format off
  if (_c < _d) {
    if (_c < r.min) r.min = _c;
    if (_d > r.max) r.max = _d;
  } else {
    if (_d < r.min) r.min = _d;
    if (_c > r.max) r.max = _c;
  }
  // clang-format on

  return r;
}

// -----------------------------------------------------------------------------
// RECTANGLE
// -----------------------------------------------------------------------------

/// 2D rectangle of single-precision floating-point numbers.
///
struct Rect {
  Vec2 min;
  Vec2 max;
};

/// Returns size of the rectangle.
///
/// @param[in] _rect Rectangle.
///
/// @return Size of the rectangle.
///
constexpr Vec2 size(const Rect& _rect) {
  return _rect.max - _rect.min;
}

/// Returns width of the rectangle.
///
/// @param[in] _rect Rectangle.
///
/// @return Width of the rectangle.
///
constexpr f32 width(const Rect& _rect) {
  return _rect.max.x - _rect.min.x;
}

/// Returns height of the rectangle.
///
/// @param[in] _rect Rectangle.
///
/// @return Height of the rectangle.
///
constexpr f32 height(const Rect& _rect) {
  return _rect.max.y - _rect.min.y;
}

/// Returns aspect ratio (as width over height) of the rectangle.
///
/// @param[in] _rect Rectangle.
///
/// @return Aspect ratio of the rectangle.
///
constexpr f32 aspect(const Rect& _rect) {
  return width(_rect) / height(_rect);
}

/// Returns center of the rectangle.
///
/// @param[in] _rect Rectangle.
///
/// @return Center of the rectangle.
///
constexpr Vec2 center(const Rect& _rect) {
  return (_rect.min + _rect.max) * 0.5f;
}

/// Returns top-left corner of the rectangle.
///
/// @param[in] _rect Rectangle.
///
/// @return Top-left corner of the rectangle.
///
constexpr Vec2 tl(const Rect& _rect) {
  return {_rect.min.x, _rect.max.y};
}

/// Returns top-right corner of the rectangle.
///
/// @param[in] _rect Rectangle.
///
/// @return Top-right corner of the rectangle.
///
constexpr Vec2 tr(const Rect& _rect) {
  return _rect.max;
}

/// Returns bottom-left corner of the rectangle.
///
/// @param[in] _rect Rectangle.
///
/// @return Bottom-left corner of the rectangle.
///
constexpr Vec2 bl(const Rect& _rect) {
  return _rect.min;
}

/// Returns bottom-right corner of the rectangle.
///
/// @param[in] _rect Rectangle.
///
/// @return Bottom-right corner of the rectangle.
///
constexpr Vec2 br(const Rect& _rect) {
  return {_rect.max.x, _rect.min.y};
}

/// Checks if an axis-aligned rectangle and a circle overlap.
///
/// @param[in] _rect    Rectangle.
/// @param[in] _center  Circle center.
/// @param[in] _radius2 Squared circle radius.
///
/// @return `true` if the rectangle and the circle overlap, `false` otherwise.
///
constexpr bool overlap(const Rect& _rect, const Vec2 _center, f32 _radius2) {
  const Vec2 closest   = clamp(_center, _rect.min, _rect.max);
  const f32  distance2 = length2(_center - closest);

  return distance2 <= _radius2;
}

/// Checks if two axis-aligned rectangles overlap.
///
/// @param[in] _a First rectangle.
/// @param[in] _b Second rectangle.
///
/// @return `true` if the rectangles overlap, `false` otherwise.
///
constexpr bool overlap(const Rect& _a, const Rect& _b) {
  return (
    _a.min.x <= _b.max.x &&
    _a.max.x >= _b.min.x &&
    _a.min.y <= _b.max.y &&
    _a.max.y >= _b.min.y);
}

// -----------------------------------------------------------------------------
// 2D TRANSFORM
// -----------------------------------------------------------------------------

/// 3x3 matrix with third row implicitly `[0 0 1]`.
/// `| a c e |`
/// `| b d f |`
/// `| 0 0 1 |`
///
struct Transform2 {
  f32 a;
  f32 b;
  f32 c;
  f32 d;
  f32 e;
  f32 f;
};

/// Multiplies two transformations.
///
/// @param[in] _M First transformation.
/// @param[in] _N Second transformation.
///
/// @return Product of the two transformations.
///
constexpr Transform2 operator*(const Transform2& _M, const Transform2& _N) {
  return {
    _M.a * _N.a + _M.c * _N.b,
    _M.b * _N.a + _M.d * _N.b,
    _M.a * _N.c + _M.c * _N.d,
    _M.b * _N.c + _M.d * _N.d,
    _M.a * _N.e + _M.c * _N.f + _M.e,
    _M.b * _N.e + _M.d * _N.f + _M.f,
  };
}

/// Transforms a vector by a transformation.
///
/// @param[in] _M Transformation.
/// @param[in] _v Vector.
///
/// @return Transformed vector.
///
constexpr Vec2 operator*(const Transform2& _M, const Vec2& _v) {
  return {
    _M.a * _v.x + _M.c * _v.y + _M.e,
    _M.b * _v.x + _M.d * _v.y + _M.f,
  };
}

/// Creates an identity transformation.
///
/// @return Identity transformation.
///
constexpr Transform2 identity() {
  return {1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f};
}

/// Creates a translation transformation.
///
/// @param[in] _x Translation in X-axis.
/// @param[in] _y Translation in Y-axis.
///
/// @return Translation transformation.
///
constexpr Transform2 translate(f32 _x, f32 _y) {
  return {1.0f, 0.0f, 0.0f, 1.0f, _x, _y};
}

/// Creates a uniform scaling transformation.
///
/// @param[in] _s Scale factor.
///
/// @return Scaling transformation.
///
constexpr Transform2 scale(f32 _s) {
  return {_s, 0.0f, 0.0f, _s, 0.0f, 0.0f};
}

/// Creates a counterclockwise rotation transformation.
///
/// @param[in] _turns Number of turns.
///
/// @return Rotation transformation.
///
inline Transform2 rotate_ccw(f32 _turns) {
  const f32 angle = _turns * 6.283185307179586f;

  const f32 c = cosf(angle);
  const f32 s = sinf(angle);

  return {c, s, -s, c, 0.0f, 0.0f};
}

/// Creates a clockwise rotation transformation.
///
/// @param[in] _turns Number of turns.
///
/// @return Rotation transformation.
///
inline Transform2 rotate_cw(f32 _turns) {
  return rotate_ccw(-_turns);
}

/// Creates a tranformation that maps one axis-aligned rectangle onto another.
/// Supports mismatched Y-axis direction (simply make the rectangle's `max.y`
/// smaller than its `min.y`).
///
/// @param[in] _src Source rectangle.
/// @param[in] _dst Destination rectangle.
///
/// @return Transformation matrix.
///
constexpr Transform2 map_rect_to_rect(const Rect& _src, const Rect& _dst) {
  const auto src_size = size(_src);
  const auto dst_size = size(_dst);

  const f32 a = dst_size.x / src_size.x;
  const f32 d = dst_size.y / src_size.y;
  const f32 e = _dst.min.x - _src.min.x * a;
  const f32 f = _dst.min.y - _src.min.y * d;

  return {a, 0.0f, 0.0f, d, e, f};
}
