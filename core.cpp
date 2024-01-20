#include "core.h"

#if (_MSC_VER)
#  include <malloc.h> // _aligned_malloc
#  define aligned_malloc _aligned_malloc
#  define aligned_free _aligned_free
#else
#  define aligned_malloc aligned_alloc
#  define aligned_free free
#endif

namespace {

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

Allocator std_alloc() {
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
      void* ptr = aligned_malloc(max(2 * sizeof(void*), size_t(_align)), size_t(_new));
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
    init_arena(arena, make_slice(buf));
    alloc = arena_alloc(arena);
  }
};

Allocator& ctx_temp_alloc() {
  thread_local DefaultTempAlloc alloc;

  return ctx_alloc();
}

void init_arena(Arena& _arena, Slice<u8>&& _buf) {
  _arena.first = begin(_buf);
  _arena.last  = end(_buf);
}

void init_arena(Arena& _arena, void* _buf, Size _bytes) {
  init_arena(_arena, make_slice((u8*)_buf, _bytes));
}

Allocator arena_alloc(Arena& _arena) {
  return {
    .ctx   = &_arena,
    .alloc = [](void* _ctx, void* _ptr, Size _old, Size _new, Size _align) -> void* {
      assert(_ctx);
      assert(is_power_of_two(_align));

      if (_new <= 0) {
        return nullptr;
      }

      Arena& arena = *(Arena*)_ctx;

      const Size available = arena.last - arena.first;
      const Size padding   = -uintptr_t(arena.first) & (_align - 1);

      panic_if(_new > available - padding, "Failed to allocate %td bytes aligned to a %td-byte boundary.", _new, _align);

      u8* ptr = arena.first + padding;
      arena.first += padding + _new;

      copy_and_zero(ptr, _new, _ptr, _old);

      return ptr;
    }};
}
