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

// If not specified, the debug mode is enabled in debug builds (when `NDEBUG`
// symbol is not defined) and disabled otherwise.
#if !defined(CORE_CONFIG_DEBUG_MODE)
#  if defined(NDEBUG)
// Debug mode is off.
#    define CORE_CONFIG_DEBUG_MODE 0
#  else
// Debug mode is on.
#    define CORE_CONFIG_DEBUG_MODE 1
#  endif
#endif

// If not specified, bounds checking is always enabled.
#if !defined(CORE_CONFIG_NO_BOUNDS_CHECK)
// Bounds checking is on.
#  define CORE_CONFIG_NO_BOUNDS_CHECK 0
#endif

// If not specified, exceptions are not thrown on panic, the program aborts.
#if !defined(CORE_CONFIG_THROW_EXCEPTION_ON_PANIC)
// Exceptions are not thrown on panic, the program aborts.
#  define CORE_CONFIG_THROW_EXCEPTION_ON_PANIC 0
#endif

// -----------------------------------------------------------------------------
// DEBUGGING
// -----------------------------------------------------------------------------

// Checks a debug-mode-only condition.
#if (CORE_CONFIG_DEBUG_MODE)
// If the condition is `false`, logs a message and breaks into the debugger.
#  define debug_assert(_cond)    \
    do {                         \
      if (!(_cond)) {            \
        ::log("assert", #_cond); \
        ::debug_break();         \
      }                          \
    } while (0)
#else
// This conditions is not checked in the current build.
#  define debug_assert(_cond) ((void)0)
#endif

// Logs a warning message.
#define warn(...) \
  ::log("warn", __VA_ARGS__)

// Logs a warning message if the condition is true.
#define warn_if(_cond, ...) \
  do {                      \
    if (_cond)              \
      warn(__VA_ARGS__);    \
  } while (0)

// If the condition is true, logs a message and panics.
#define panic_if(_cond, ...) \
  do {                       \
    if (_cond)               \
      ::panic(__VA_ARGS__);  \
  } while (0)

// Bounds checking.
#if (CORE_CONFIG_NO_BOUNDS_CHECK)
// Bounds checking is off.
#  define check_bounds(_cond) ((void)0)
#else
// Bounds checking is on.
#  define check_bounds(_cond) \
    panic_if(!(_cond), "Bounds check failure: %s", #_cond)
#endif

// Unconditionally breaks into the debugger.
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

// Basic exception information. Thrown on panic if the library is built with
// `CORE_CONFIG_THROW_EXCEPTION_ON_PANIC` set to a non-zero value.
struct Exception {
  const char* file;
  int         line;
  char        msg[256];
};

namespace detail {

// Helper struct for retaining the debugging information. Can be passed a string
// literal and it automatically captures the file and line number, without using
// macros.
struct Fmt {
  const char* fmt;
  const char* file;
  int         line;

  // Constructs the format instance, automatically capturing the file and line
  // number. The required built-ins are supported across the board in the major
  // recent-ish compilers (written as of 2024).
  constexpr Fmt(const char* _fmt, const char* _file = __builtin_FILE(), int _line = __builtin_LINE())
    : fmt(_fmt), file(_file), line(_line) {}
};

} // namespace detail

// Logs a generic message.
void log(const char* _kind, detail::Fmt _fmt, ...);

// Unconditionally prints the message and panics, i.e., aborts, unless compiled
// with `CORE_CONFIG_THROW_EXCEPTION_ON_PANIC`.
[[noreturn]] void panic(detail::Fmt _fmt, ...);

// -----------------------------------------------------------------------------
// SIZED TYPES
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

// Tag that indicates that a function parameter is unspecified or omitted.
struct OmittedTag {};

// Constant that can be used to indicate that a function parameter is omitted,
// e.g., `foo(_, 42)`.
constexpr OmittedTag _;

// -----------------------------------------------------------------------------
// TYPE TRAITS
// -----------------------------------------------------------------------------

namespace detail {

// Provides member typedef type, which is defined as `TrueType` if the condition
// is `true` at compile time, or as `FalseType` if the condition is `false`.
template <bool Condition, typename TrueType, typename FalseType>
struct Conditional {
  using Type = TrueType;
};

// Specialization for the case when the condition evaluates as `false`.
template <class TrueType, typename FalseType>
struct Conditional<false, TrueType, FalseType> {
  using Type = FalseType;
};

// Helper struct for removing the constant qualifier from a type.
template <typename T>
struct RemoveConst {
  using Type = T;
};

// Helper struct for removing the constant qualifier from a type.
template <typename T>
struct RemoveConst<const T> {
  using Type = T;
};

// Helper struct for checking if two types are the same.
template <typename T, typename U>
struct TypeEquivalence {
  static constexpr bool val = false;
};

// Helper struct for checking if two types are the same.
template <typename T>
struct TypeEquivalence<T, T> {
  static constexpr bool val = true;
};

// Helper struct used to generate type-specific static variable, whose address
// is used as a unique identifier for the type.
template <typename T>
struct TypeId {
  static constexpr int val = 0;
};

// Returns unique type identifier for type `T`.
template <typename T>
constexpr uintptr_t type_id() {
  return uintptr_t(&detail::TypeId<T>::val);
}

// Returns unique type identifier for the `nullptr` type.
template <>
constexpr uintptr_t type_id<decltype(nullptr)>() {
  return uintptr_t(0);
}

} // namespace detail

// Removes the constant qualifier from the type.
template <typename T>
using NonConst = typename detail::RemoveConst<T>::Type;

// Adds the constant qualifier to the type.
template <typename T>
using Const = const typename detail::RemoveConst<T>::Type;

// Constant that is `true` if the two types are the same, `false` otherwise.
template <typename T, typename U>
constexpr bool is_same = detail::TypeEquivalence<T, U>::val;

// Unique identifier for a type. Not human-readable.
template <typename T>
uintptr_t TypeId = detail::type_id<T>();

// -----------------------------------------------------------------------------
// NON COPYABLE BASE
// -----------------------------------------------------------------------------

// Base class that makes derived classes non-copyable.
struct NonCopyable {
  NonCopyable() = default;

  NonCopyable(const NonCopyable&) = delete;

  NonCopyable& operator=(const NonCopyable&) = delete;
};

// -----------------------------------------------------------------------------
// DEFERRED EXECUTION
// -----------------------------------------------------------------------------

namespace detail {

// Stores a function to be executed when the object goes out of scope.
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

// Defers execution of the passed code until the end of the current scope.
#define defer(...) \
  ::detail::Deferred CONCAT(deferred_, __LINE__)([&]() mutable { __VA_ARGS__; })

// -----------------------------------------------------------------------------
// SMALL UTILITIES
// -----------------------------------------------------------------------------

// KiB literal.
constexpr Size operator""_KiB(unsigned long long _x) {
  return 1024 * Size(_x);
}

// MiB literal.
constexpr Size operator""_MiB(unsigned long long _x) {
  return 1024 * 1024 * Size(_x);
}

// GiB literal.
constexpr Size operator""_GiB(unsigned long long _x) {
  return 1024 * 1024 * 1024 * Size(_x);
}

// Returns the number of elements in a statically-sized array.
template <typename T, Size N>
constexpr Size countof(const T (&)[N]) noexcept {
  return N;
}

// Returns the minimum of two values.
template <typename T>
constexpr T min(T _x, T _y) {
  return _x < _y ? _x : _y;
}

// Returns the maximum of two values.
template <typename T>
constexpr T max(T _x, T _y) {
  return _x > _y ? _x : _y;
}

// Clamps the input value to the specified range (inclusive).
template <typename T>
constexpr T clamp(T _x, T _min, T _max) {
  return min(max(_x, _min), _max);
}

// Swaps two values.
template <typename T>
constexpr void swap(T& _x, T& _y) {
  T tmp = _x;
  _x    = _y;
  _y    = tmp;
}

// Checks if the input value is a power of two.
template <typename T>
constexpr bool is_power_of_two(T _value) {
  return (_value & (_value - 1)) == 0;
}

// Aligns a value up to the specified power-of-two alignment.
template <typename T, typename A>
constexpr T align_up(T _value, A _align) {
  return (_value + _align - 1) & ~(_align - 1);
}

// Aligns a pointer address up to the specified power-of-two alignment.
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

// Returns the input value if it is greater than the threshold, otherwise the
// default value.
template <typename T>
constexpr T gtr_or(T _x, T _threshold, T _default) {
  return _x > _threshold ? _x : _default;
}

// Packs a floating-point number into a 16-bit unsigned integer.
constexpr u16 pack_as_u16(f32 _x, f32 _scale, f32 _shift) {
  return u16((_x + _shift) * _scale + 0.5f);
}

// Packs two floating-point numbers into a 32-bit unsigned integer. Each number
// is packed into 16 bits.
constexpr u32 pack_as_2xu_16(f32 _x, f32 _y, f32 _scale, f32 _shift) {
  return (pack_as_u16(_y, _scale, _shift) << 16) | pack_as_u16(_x, _scale, _shift);
}

// -----------------------------------------------------------------------------
// TUPLE
// -----------------------------------------------------------------------------

// Basic tuple of two values. Used to ease returning multiple values from a
// function.
template <typename T0, typename T1>
struct Tuple2 {
  T0 val0;
  T1 val1;
};

// Basic tuple of three values. Used to ease returning multiple values from a
// function.
template <typename T0, typename T1, typename T2>
struct Tuple3 {
  T0 val0;
  T1 val1;
  T1 val2;
};

// -----------------------------------------------------------------------------
// ANY PTR
// -----------------------------------------------------------------------------

// Type-erased pointer storage that can be safely type-cast back to the original
// type. Casting to a different type is not allowed and will cause a panic.
struct AnyPtr {
  const void* ptr  = nullptr; // Pointer to the data.
  uintptr_t   type = 0;       // Unique identifier for the type.

  // Constructs an empty pointer.
  AnyPtr() = default;

  // Constructs a pointer from a null pointer literal.
  AnyPtr(decltype(nullptr))
    : ptr(nullptr), type(0) {}

  // Constructs a pointer from a typed pointer.
  template <typename T>
  AnyPtr(T* _ptr)
    : ptr(_ptr), type(TypeId<T>) {}

  // Checks if the pointer is not null.
  operator bool() const {
    return ptr != nullptr;
  }

  // Casts the pointer to the specified type. Panics if the type does not match
  // the original type.
  template <typename T>
  T* as() const {
    panic_if(type != TypeId<T> && type != TypeId<NonConst<T>>, "Failed to safely type-cast AnyPtr.");
    return static_cast<T*>(const_cast<void*>(ptr));
  }
};

// -----------------------------------------------------------------------------
// ALLOCATORS
// -----------------------------------------------------------------------------

// Allocator interface. Used to abstract memory handling.
struct Allocator {
  // Allocator flags.
  enum Flags : u8 {
    DEFAULT  = 0x0, // Default behavior.
    FREE_ALL = 0x1, // Free all owned memory. Not every allocator supports this.
    NON_ZERO = 0x2, // Do not explicitely zero-initialize allocated memory.
    NO_PANIC = 0x4, // Do not panic on allocation failure. Return `nullptr`.
  };

  // Optional allocator-specific context.
  AnyPtr ctx;

  // Allocator callback. Behaves as stdlib's `realloc`.
  void* (*alloc)(AnyPtr& _ctx, void* _ptr, Size _old, Size _new, Size _align, u8 _flags);
};

// Returns allocator interface that uses stdlib's heap allocation routines.
// Intended for generic use.
Allocator std_alloc();

// Returns a thread-local reference to current context's allocator. By default,
// this is the stdlib heap allocator.
Allocator& ctx_alloc();

// Returns a thread-local reference to current context's temporary allocator.
// A temporary allocator is used for short-lived allocations, which are freed
// once per "cycle" (e.g., per frame).
Allocator& ctx_temp_alloc();

namespace detail {

// Helper struct managing the scoped "allocator push/pop" logic.
struct ScopedAllocator : NonCopyable {
  Allocator prev;

  ScopedAllocator(const Allocator& _alloc)
    : prev(ctx_alloc()) { ctx_alloc() = _alloc; }

  ~ScopedAllocator() { ctx_alloc() = prev; }
};

} // namespace detail

// Sets the current context's allocator to the provided allocator. The previous
// allocator is restored when the scope is exited.
#define scope_alloc(_alloc) \
  ::detail::ScopedAllocator CONCAT(scoped_alloc_, __LINE__)(_alloc)

// -----------------------------------------------------------------------------
// ALLOCATION HELPERS
// -----------------------------------------------------------------------------

// Reallocates a memory block using the provided allocator. This is the most
// generic allocation function. Has same semantics as `realloc` but requires
// the caller to provide additional information, such as the old memory size,
// and alignment, as the allocator is not required to track this information.
void* reallocate(Allocator& _alloc, void* _ptr, Size _old, Size _new, Size _align, u8 _flags = Allocator::DEFAULT);

// Reallocates a memory block using the current context's allocator.
void* reallocate(void* _ptr, Size _old, Size _new, Size _align, u8 _flags = Allocator::DEFAULT);

// Allocates a new memory block using the provided allocator.
void* allocate(Allocator& _alloc, Size _size, Size _align, u8 _flags = Allocator::DEFAULT);

// Allocates a new memory block using the current context's allocator.
void* allocate(Size _size, Size _align, u8 _flags = Allocator::DEFAULT);

// Frees a memory block using the provided allocator.
void free(Allocator& _alloc, void* _ptr, Size _size);

// Frees a memory block using the current context's allocator.
void free(void* _ptr, Size _size);

// Frees all memory allocated using the provided allocator.
void free_all(Allocator& _alloc);

// Frees all memory allocated using the current context's allocator.
void free_all();

// -----------------------------------------------------------------------------
// SLICE
// -----------------------------------------------------------------------------

namespace detail {

// Helper type to facilitate indexing from the back of a slice.
struct BackIndex {
  Index val; // Offset from the end of the slice.
};

} // namespace detail

// Constant that can be used to index from the back of a slice, e.g.,
// `foo[len - 1]`.
constexpr detail::BackIndex len = {};

// Offsets the back index by the specified amount.
constexpr detail::BackIndex operator-(detail::BackIndex _i, Index _j) {
  return {_i.val - _j};
}

// Non-owning memory slice.
template <typename T>
struct Slice {
  T*   data; // Pointer to the first element.
  Size len;  // Number of elements.

  // Checks if the slice is not empty.
  constexpr operator bool() const {
    return data != nullptr;
  }

  // Converts the slice to a slice of constant elements.
  constexpr operator Slice<Const<T>>() const {
    return {
      .data = data,
      .len  = len,
    };
  }

  // Returns i-th element of the slice.
  T& operator[](Index _i) const {
    check_bounds(_i >= 0 && _i < len);
    return data[_i];
  }

  // Returns i-th element from the back of the slice.
  T& operator[](detail::BackIndex _i) const {
    return operator[](len + _i.val);
  }

  // Returns a subslice of the slice. `_low` is inclusive, `_high` is exclusive.
  Slice<T> operator()(Index _low, Index _high) const {
    check_bounds(_low >= 0);
    check_bounds(_low <= _high);
    check_bounds(_high <= len);
    return {
      .data = data + _low,
      .len  = _high - _low,
    };
  }

  // Returns a subslice of the slice. `_low` is implicitly 0.
  Slice<T> operator()(OmittedTag, Index _high) const {
    return operator()(0, _high);
  }

  // Returns a subslice of the slice. `_high` is implicitly `len`.
  Slice<T> operator()(Index _low, OmittedTag) const {
    return operator()(_low, len);
  }

  // Returns a subslice of the slice. `_low` is indexed from the back of the
  // slice. `_high` is implicitly `len`.
  Slice<T> operator()(detail::BackIndex _low, OmittedTag) const {
    return operator()(len + _low.val, len);
  }
};

// Makes a non-owning slice, wrapping a statically-sized array.
template <typename T, Size N>
constexpr Slice<T> make_slice(T (&_array)[N]) {
  Slice<T> slice;
  slice.data = _array;
  slice.len  = N;

  return slice;
}

namespace detail {

template <typename T, Size N, auto Unique>
struct StackStorage {
  T data[N];
};

} // namespace detail

// Makes a non-owning slice, wrapping a statically-sized array literal. See
// https://github.com/DaemonSnake/unconstexpr-cpp20?tab=readme-ov-file#unique-instantiation-template
// for the rationale behind the `Unique` parameter. It is used to ensure that
// we can create unique stack memory for every call of the function, even if the
// type `T` and the size `N` are the same.
//
// Note that array / compound literals aren't standard C++, but they are
// somewhat supported by most compilers. On MSVC, you need to compile with
// `/wd4576` if you want to use this function (if you use the CMake setup, then
// this is already done for you).
template <typename T, Size N, auto Unique = [] {}>
Slice<T> make_slice(T (&&_array)[N]) {
  static detail::StackStorage<T, N, Unique> storage{};
  memcpy(storage.data, _array, N * sizeof(T));

  Slice<T> slice;
  slice.data = storage.data;
  slice.len  = N;

  return slice;
}

// Makes a non-owning slice, wrapping a pointer to the first element and the
// associated number of elements.
template <typename T>
constexpr Slice<T> make_slice(T* _data, Size _len) {
  Slice<T> slice;
  slice.data = _data;
  slice.len  = _len;

  return slice;
}

// Explicitly deleted slice construction from a null pointer literal.
template <typename T>
constexpr Slice<T> make_slice(decltype(nullptr), Size) = delete;

// Copies elements from `_src` slice to `_dst` slice. Copies the minimum of the
// two slice lengths.
template <typename T>
void copy(Slice<T> _dst, Slice<Const<T>> _src) {
  memcpy(_dst.data, _src.data, size_t(min(_dst.len, _src.len)) * sizeof(T));
}

// Returns `true` if the slice is empty, `false` otherwise.
template <typename T>
constexpr bool empty(Slice<T> _slice) {
  debug_assert(_slice.len >= 0);
  return _slice.len == 0;
}

// Returns a pointer to the first element of the slice.
template <typename T>
constexpr T* begin(Slice<T> _slice) {
  return _slice.data;
}

// Returns a pointer to the "one-past-the-end" element of the slice.
template <typename T>
constexpr T* end(Slice<T> _slice) {
  return _slice.data + _slice.len;
}

// Returns the number of bytes used by the slice.
template <typename T>
constexpr size_t bytes(Slice<T> _slice) {
  return size_t(_slice.len) * sizeof(T);
}

// -----------------------------------------------------------------------------
// DYNAMIC ARRAY
// -----------------------------------------------------------------------------

namespace detail {

// Helper function to calculate the next capacity for a dynamic slice.
constexpr Size next_cap(Size _cap, Size _req) {
  return max(max(Size(8), _req), (_cap * 3) / 2);
}

} // namespace detail

// Dynamic (growable) array of elements of type `T`. Owns the memory it points
// to. Retains a pointer to the allocator that was used to allocate its memory.
template <typename T>
struct Array : Slice<T> {
  Size       cap;
  Allocator* alloc;
};

// Makes a dynamic array of specified length and capacity, using the provided
// allocator.
template <typename T>
Array<T> make_array(Size _len, Size _cap, Allocator& _alloc) {
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

// Makes a dynamic array of specified length and capacity, using the current
// context's allocator.
template <typename T, typename S>
Array<T> make_array(S _len, Size _cap) {
  static_assert(is_same<S, Size> || is_same<S, decltype(0)>);

  return make_array<T>(Size(_len), _cap, ctx_alloc());
}

// Makes a dynamic array of specified length, using the current context's
// allocator. Capacity is set equal to the length.
template <typename T>
Array<T> make_array(Size _len) {
  return make_array<T>(_len, _len);
}

// Makes a dynamic array of specified length, using the provided allocator.
// Capacity is set equal to the length.
template <typename T>
Array<T> make_array(Size _len, Allocator& _alloc) {
  return make_array<T>(_len, _len, _alloc);
}

// Destroys the dynamic array and frees the memory it points to.
template <typename T>
void destroy(Array<T>& _array) {
  free(*_array.alloc, _array.data, _array.cap * sizeof(T));
}

// Ensures that the array has enough capacity to store at least `_cap` elements.
template <typename T>
void reserve(Array<T>& _array, Size _cap) {
  if (_cap <= _array.cap) {
    return;
  }

  _array.data = (T*)reallocate(*_array.alloc, _array.data, _array.cap * sizeof(T), _cap * sizeof(T), alignof(T));
  _array.cap  = _cap;
}

// Resizes the array to the specified length.
template <typename T>
void resize(Array<T>& _array, Size _len) {
  debug_assert(_len >= 0);

  if (_len <= _array.len) {
    _array.len = _len;
    return;
  }

  if (_len <= _array.cap) {
    memset(_array.data + _array.len, 0, size_t(_len - _array.len) * sizeof(T));
    _array.len = _len;
    return;
  }

  reserve(_array, detail::next_cap(_array.cap, _len));
  _array.len = _len;
}

// Appends a single value to the dynamic array.
template <typename T>
void append(Array<T>& _array, const T& _value) {
  if (_array.len == _array.cap) {
    reserve(_array, detail::next_cap(_array.cap, _array.len + 1));
  }

  _array.data[_array.len++] = _value;
}

// Appends multiple values to the dynamic array.
template <typename T>
void append(Array<T>& _array, Slice<Const<T>> _values) {
  const Size low  = _array.len;
  const Size high = _array.len + _values.len;

  if (high >= _array.cap) {
    reserve(_array, detail::next_cap(_array.cap, high));
  }

  _array.len = high;
  copy(_array(low, high), _values);
}

// Removes the last element from the dynamic array.
template <typename T>
T& pop(Array<T>& _array) {
  check_bounds(_array.len > 0);
  return _array.data[--_array.len];
}

// Removes the i-th element from the dynamic array and shifts the rest of the
// elements to the left, thus preserving the order.
template <typename T>
void remove_ordered(Array<T>& _array, Index _i) {
  check_bounds(_i >= 0 && _i < _array.len);

  if (_i != _array.len - 1) {
    memmove(_array.data + _i, _array.data + _i + 1, size_t(_array.len - _i - 1) * sizeof(T));
  }

  _array.len--;
}

// Removes the i-th element from the dynamic array, without preserving the
// order.
template <typename T>
void remove_unordered(Array<T>& _array, Index _i) {
  check_bounds(_i >= 0 && _i < _array.len);

  if (_i != _array.len - 1) {
    _array.data[_i] = _array.data[_array.len - 1];
  }

  _array.len--;
}

// -----------------------------------------------------------------------------
// SIMPLE ARENA (NON-OWNING)
// -----------------------------------------------------------------------------

// Simple fixed-size arena that allocates memory from a single non-owned buffer.
struct Arena {
  Slice<u8> buf;  // Non-owned backing memory.
  Index     head; // Offset to the next free byte.
};

// Creates an arena using the provided buffer.
Arena make_arena(Slice<u8> _buf);

// Makes an allocator interface using the provided arena. The arena must live at
// least as long as the allocator.
Allocator make_alloc(Arena& _arena);

// -----------------------------------------------------------------------------
// SLAB ARENA (GROWABLE)
// -----------------------------------------------------------------------------

// Simple growable arena that allocates memory in fixed-sized, owned slabs.
struct SlabArena {
  Array<Slice<u8>> slabs; // Owned slabs. The first slab is always allocated.
  Index            head;  // Index of the active slab.
};

// Creates a slab arena using the provided allocator and slab size.
SlabArena make_slab_arena(Allocator& _alloc, Size _slab_size);

// Creates a slab arena using the provided allocator and default slab size.
SlabArena make_slab_arena(Allocator& _alloc);

// Creates a slab arena using the current context's allocator and default slab
// size.
SlabArena make_slab_arena();

// Destroys the slab arena and frees the memory it owns.
void destroy(SlabArena& _arena);

// Makes an allocator interface using the provided slab arena.  The arena must
// live at least as long as the allocator.
Allocator make_alloc(SlabArena& _arena);

// -----------------------------------------------------------------------------
// RING BUFFER
// -----------------------------------------------------------------------------

// Simple fixed-size ring buffer that allocates memory from a single non-owned
// backing buffer.
template <typename T>
struct Ring {
  Slice<T> buf;  // Non-owned backing memory.
  Index    head; // Index of the next element to be added.
  Index    tail; // Index of the oldest element (next one to be removed).
};

// Creates a ring buffer using the backing memory.
template <typename T>
Ring<T> make_ring(Slice<T> _buf) {
  debug_assert(_buf.len > 1);

  return {
    .buf  = {_buf.data, _buf.len},
    .head = 0,
    .tail = 0,
  };
}

// Adds a new element to the ring buffer.
template <typename T>
void push(Ring<T>& _ring, const T& _value) {
  const Size head = (_ring.head + 1) % _ring.buf.len;
  check_bounds(head != _ring.tail);

  _ring.buf[_ring.head] = _value;
  _ring.head            = head;
}

// Removes and returns the oldest element from the ring buffer.
template <typename T>
T& pop(Ring<T>& _ring) {
  check_bounds(_ring.head != _ring.tail);

  T& elem    = _ring.buf[_ring.tail];
  _ring.tail = (_ring.tail + 1) % _ring.buf.len;

  return elem;
}

// Returns `true` if the ring buffer is empty, `false` otherwise.
template <typename T>
constexpr bool empty(const Ring<T>& _ring) {
  return _ring.head == _ring.tail;
}

// -----------------------------------------------------------------------------
// RANGE UTILITIES
// -----------------------------------------------------------------------------

// Reverses the order of the elements in the slice.
template <typename T>
void reverse(Slice<T> _slice) {
  if (empty(_slice)) {
    return;
  }

  Index i = 0;
  Index j = _slice.len;

  for (--j; i < j; i++, --j) {
    swap(_slice[i], _slice[j]);
  }
}

// Merges two sorted sequences into a single sorted sequence using the provided
// comparison function. The destination sequence must have enough capacity to
// hold all elements from both source sequences. The comparator function must
// return `true` if the first argument should precede the second one in the
// merged sequence.
//
// Note that the three different types are needed as apparently the compiler
// cannot deduce the type if we do something like
// `Slice<NonConst<T>> _dst, Slice<Const<T>> _src_0, Slice<Const<T>> _src_1`.
template <typename T, typename U, typename V, typename Comp>
void merge(Slice<T> _dst, Slice<U> _src_0, Slice<V> _src_1, Comp _comp) {
  panic_if(_dst.len < _src_0.len + _src_1.len, "Destination sequence has insufficient capacity.");

  Index i0 = 0;
  Index i1 = 0;
  Index d  = 0;

  for (; i0 < _src_0.len; d++) {
    if (i1 == _src_1.len) {
      copy(_dst(d, _), _src_0(i0, _));
      break;
    }

    if (_comp(_src_1[i1], _src_0[i0])) {
      _dst[d] = _src_1[i1++];
    } else {
      _dst[d] = _src_0[i0++];
    }
  }

  copy(_dst(d, _), _src_1(i1, _));
}

template <typename T>
void merge(Slice<T> _dst, Slice<const T> _src_0, Slice<const T> _src_1) {
  merge(_dst, _src_0, _src_1, [](const T& _a, const T& _b) { return _a < _b; });
}

// -----------------------------------------------------------------------------
// FILE I/O
// -----------------------------------------------------------------------------

// Reads the entire file into a dynamic slice of bytes. Uses the provided
// allocator.
Array<u8> read_bytes(const char* _path, Allocator& _alloc);

// Reads the entire file into a dynamic slice of bytes. Uses the current
// context's allocator
Array<u8> read_bytes(const char* _path);

// Reads the entire file into a dynamic slice of characters and appends a null
// terminator. Uses the provided allocator.
Array<char> read_string(const char* _path, Allocator& _alloc);

// Reads the entire file into a dynamic slice of characters and appends a null
// terminator. Uses the current context's allocator.
Array<char> read_string(const char* _path);

// -----------------------------------------------------------------------------
// 2D VECTOR
// -----------------------------------------------------------------------------

// 2D vector of single-precision floating-point numbers.
struct Vec2 {
  f32 x; // X component.
  f32 y; // Y component.
};

// Negates (flips) a vector.
constexpr Vec2 operator-(Vec2 _u) {
  return {-_u.x, -_u.y};
}

// Adds two vectors.
constexpr Vec2 operator+(Vec2 _u, Vec2 _v) {
  return {_u.x + _v.x, _u.y + _v.y};
}

// Subtracts two vectors.
constexpr Vec2 operator-(Vec2 _u, Vec2 _v) {
  return {_u.x - _v.x, _u.y - _v.y};
}

// Multiplies a vector by a scalar.
constexpr Vec2 operator*(Vec2 _u, f32 _s) {
  return {_u.x * _s, _u.y * _s};
}

// Computes vector's squared length.
constexpr f32 length2(Vec2 _v) {
  return _v.x * _v.x + _v.y * _v.y;
}

// Computes vector's length.
inline f32 length(Vec2 _v) {
  return sqrtf(length2(_v));
}

// Returns vector's minimum component.
constexpr f32 min(Vec2 _v) {
  return min(_v.x, _v.y);
}

// Returns vector with minimum components.
constexpr Vec2 min(Vec2 _u, Vec2 _v) {
  return {min(_u.x, _v.x), min(_u.y, _v.y)};
}

// Returns vector's maximum component.
constexpr f32 max(Vec2 _v) {
  return max(_v.x, _v.y);
}

// Returns vector with maximum components.
constexpr Vec2 max(Vec2 _u, Vec2 _v) {
  return {max(_u.x, _v.x), max(_u.y, _v.y)};
}

// Returns a normalized copy of the vector (with unit length).
inline Vec2 normalized(Vec2 _v) {
  return _v * (1.0f / length(_v));
}

// Returns a copy of the vector rotated by 90 degrees clockwise.
constexpr Vec2 perpendicular_cw(Vec2 _v) {
  return {_v.y, -_v.x};
}

// Returns a copy of the vector rotated by 90 degrees counterclockwise.
constexpr Vec2 perpendicular_ccw(Vec2 _v) {
  return {-_v.y, _v.x};
}

// Computes dot product of two vectors.
constexpr f32 dot(Vec2 _u, Vec2 _v) {
  return _u.x * _v.x + _u.y * _v.y;
}

// Returns cross product of two 2D vectors (equal to the area of the
// parallelogram spanned by them).
constexpr f32 cross2(Vec2 _a, Vec2 _b) {
  return _a.x * _b.y - _a.y * _b.x;
}

// -----------------------------------------------------------------------------
// 1D RANGE
// -----------------------------------------------------------------------------

// 1D range of single-precision floating-point numbers. The range is inclusive.
struct Range {
  f32 min; // Lower bound.
  f32 max; // Upper bound.
};

// Checks if two ranges overlap. Assumes that each range's minimum is less than
// or equal to its maximum.
constexpr bool overlap(Range _a, Range _b) {
  return (_a.min <= _b.max) && (_b.min <= _a.max);
}

// Constructs a range from two numbers, assignig the smaller number to `min`
// and the larger number to `max`.
constexpr Range min_max(f32 _a, f32 _b) {
  return _a < _b ? Range{_a, _b} : Range{_b, _a};
}

// Constructs a range from four numbers, assignig the smallest number to `min`
// and the largest number to `max`.
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

// 2D rectangle of single-precision floating-point numbers.
struct Rect {
  Vec2 min; // Bottom-left corner.
  Vec2 max; // Top-right corner.
};

// Returns size of the rectangle.
constexpr Vec2 size(const Rect& _rect) {
  return _rect.max - _rect.min;
}

// Returns width of the rectangle.
constexpr f32 width(const Rect& _rect) {
  return _rect.max.x - _rect.min.x;
}

// Returns height of the rectangle.
constexpr f32 height(const Rect& _rect) {
  return _rect.max.y - _rect.min.y;
}

// Returns aspect ratio (as width over height) of the rectangle.
constexpr f32 aspect(const Rect& _rect) {
  return width(_rect) / height(_rect);
}

// Returns center of the rectangle.
constexpr Vec2 center(const Rect& _rect) {
  return (_rect.min + _rect.max) * 0.5f;
}

// Returns top-left corner of the rectangle.
constexpr Vec2 tl(const Rect& _rect) {
  return {_rect.min.x, _rect.max.y};
}

// Returns top-right corner of the rectangle.
constexpr Vec2 tr(const Rect& _rect) {
  return _rect.max;
}

// Returns bottom-left corner of the rectangle.
constexpr Vec2 bl(const Rect& _rect) {
  return _rect.min;
}

// Returns bottom-right corner of the rectangle.
constexpr Vec2 br(const Rect& _rect) {
  return {_rect.max.x, _rect.min.y};
}

// Checks if an axis-aligned rectangle and a circle overlap.
constexpr bool overlap(const Rect& _rect, const Vec2 _center, f32 _radius2) {
  const Vec2 closest   = clamp(_center, _rect.min, _rect.max);
  const f32  distance2 = length2(_center - closest);

  return distance2 <= _radius2;
}

// Checks if two axis-aligned rectangles overlap.
constexpr bool overlap(const Rect& _a, const Rect& _b) {
  return (
    _a.min.x <= _b.max.x &&
    _a.max.x >= _b.min.x &&
    _a.min.y <= _b.max.y &&
    _a.max.y >= _b.min.y);
}

// Checks if a point is inside an axis-aligned rectangle.
constexpr bool inside(Vec2 _p, Rect _r) {
  return (
    (_p.x >= _r.min.x && _p.x <= _r.max.x) &&
    (_p.y >= _r.min.y && _p.y <= _r.max.y));
}

// -----------------------------------------------------------------------------
// 2D LINE SEGMENT
// -----------------------------------------------------------------------------

// 2D line segment.
struct LineSeg {
  Vec2 p0; // Start point.
  Vec2 p1; // End point.
};

// Computes line segment's length.
inline f32 length(LineSeg _l) {
  return length(_l.p1 - _l.p0);
}

// Checks if two line segments intersect.
constexpr bool intersect(LineSeg _l0, LineSeg _l1) {
  const Vec2 a = _l0.p1 - _l0.p0;
  const Vec2 b = _l1.p1 - _l1.p0;
  const Vec2 c = _l0.p0 - _l1.p0;

  const f32 d = cross2(a, b);
  if (d == 0.0f) {
    return false;
  }

  const f32 s = cross2(a, c) / d;
  const f32 t = cross2(b, c) / d;

  return (s >= 0.0f && s <= 1.0f && t >= 0.0f && t <= 1.0f);
}

// Checks if a line segment and an axis-aligned rectangle intersect.
constexpr bool intersect(LineSeg _l, Rect _r) {
  if (inside(_l.p0, _r) || inside(_l.p1, _r)) {
    return true;
  }

  return (
    intersect(_l, LineSeg{tl(_r), tr(_r)}) ||
    intersect(_l, LineSeg{tr(_r), br(_r)}) ||
    intersect(_l, LineSeg{br(_r), bl(_r)}) ||
    intersect(_l, LineSeg{bl(_r), tl(_r)}));
}

// -----------------------------------------------------------------------------
// 2D TRANSFORM
// -----------------------------------------------------------------------------

// 3x3 2D transformation matrix with third row implicitly `[0 0 1]`.
// `| a c e |`
// `| b d f |`
// `| 0 0 1 |`
struct Transform2 {
  f32 a; // Element at row 1, column 1.
  f32 b; // Element at row 2, column 1.
  f32 c; // Element at row 1, column 2.
  f32 d; // Element at row 2, column 2.
  f32 e; // Element at row 1, column 3.
  f32 f; // Element at row 2, column 3.
};

// Multiplies two transformation matrices.
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

// Transforms a vector by a transformatio matrix.
constexpr Vec2 operator*(const Transform2& _M, const Vec2& _v) {
  return {
    _M.a * _v.x + _M.c * _v.y + _M.e,
    _M.b * _v.x + _M.d * _v.y + _M.f,
  };
}

// Creates an identity transformation matrix.
constexpr Transform2 identity() {
  return {1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f};
}

// Creates a translation transformation matrix.
constexpr Transform2 translate(f32 _x, f32 _y) {
  return {1.0f, 0.0f, 0.0f, 1.0f, _x, _y};
}

// Creates a uniform scaling transformation matrix.
constexpr Transform2 scale(f32 _s) {
  return {_s, 0.0f, 0.0f, _s, 0.0f, 0.0f};
}

// Creates a counterclockwise-rotation transformation matrix.
inline Transform2 rotate_ccw(f32 _turns) {
  const f32 angle = _turns * 6.283185307179586f;

  const f32 c = cosf(angle);
  const f32 s = sinf(angle);

  return {c, s, -s, c, 0.0f, 0.0f};
}

// Creates a clockwise-rotation transformation matrix.
inline Transform2 rotate_cw(f32 _turns) {
  return rotate_ccw(-_turns);
}

// Creates a tranformation matrix that maps one axis-aligned rectangle onto
// another. Supports mismatched Y-axis direction (simply make the rectangle's
// `max.y` smaller than its `min.y`).
constexpr Transform2 remap_rects(const Rect& _from, const Rect& _to) {
  const auto src_size = size(_from);
  const auto dst_size = size(_to);

  const f32 a = dst_size.x / src_size.x;
  const f32 d = dst_size.y / src_size.y;
  const f32 e = _to.min.x - _from.min.x * a;
  const f32 f = _to.min.y - _from.min.y * d;

  return {a, 0.0f, 0.0f, d, e, f};
}
