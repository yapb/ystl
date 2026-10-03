// SPDX-License-Identifier: Unlicense

#pragma once

#include <ystl/atomic.h>
#include <ystl/movable.h>
#include <ystl/fixedarray.h>
#include <ystl/platform.h>
#include <ystl/singleton.h>

#include <cstddef>

#if defined(YSTL_LINUX) || defined(YSTL_ANDROID)
  #include <malloc.h>
#elif defined(YSTL_MACOS)
  #include <malloc/malloc.h>
#endif

#if defined(YSTL_DEBUG) && (defined(YSTL_LINUX) || defined(YSTL_ANDROID) || defined(YSTL_MACOS))
  #include <pthread.h>
#endif

#if defined(YSTL_CXX_MSVC)
  #include <intrin.h>
#endif

namespace ystl {

// internal memory manager
namespace mem {

#if defined(YSTL_DEBUG)
  // address of the current function's return address slot
  #if defined(YSTL_CXX_MSVC)
    #define YSTL_MEM_FRAME_BASE reinterpret_cast<void **> (_AddressOfReturnAddress ())
  #else
    #define YSTL_MEM_FRAME_BASE reinterpret_cast<void **> (__builtin_frame_address (0))
  #endif

// walks the frame chain and returns the address at given depth
YSTL_FORCE_INLINE const void *stack_address ([[maybe_unused]] void **base, const size_t depth) noexcept {
  #if defined(YSTL_WINDOWS)
  void *captured[32] {};
  const USHORT count = RtlCaptureStackBackTrace (0, 32, captured, nullptr);

  // depth 0 addresses the request site, which is the first captured frame
  return depth < static_cast<size_t> (count) ? captured[depth] : static_cast<const void *> (nullptr);
  #else
  auto stack_bounds = [] () noexcept {
    struct Limits {
      uintptr_t low = 0;
      uintptr_t high = 0;
    };
    Limits limits {};

    #if defined(YSTL_MACOS)
    // macos has no pthread_getattr_np, query the stack bounds directly
    auto base = static_cast<char *> (pthread_get_stackaddr_np (pthread_self ()));
    const auto size = pthread_get_stacksize_np (pthread_self ());

    if (base != nullptr && size > 0) {
      limits.low = reinterpret_cast<uintptr_t> (base - size);
      limits.high = reinterpret_cast<uintptr_t> (base);
    }
    #elif defined(YSTL_LINUX) || defined(YSTL_ANDROID)
    // covers the main thread too, unlike a cached initial-stack guess
    pthread_attr_t attr {};

    if (pthread_getattr_np (pthread_self (), &attr) == 0) {
      void *base = nullptr;
      size_t size = 0;

      if (pthread_attr_getstack (&attr, &base, &size) == 0 && base != nullptr && size > 0) {
        limits.low = reinterpret_cast<uintptr_t> (base);
        limits.high = reinterpret_cast<uintptr_t> (static_cast<char *> (base) + size);
      }
      pthread_attr_destroy (&attr);
    }
    #endif
    // other platforms (or query failure) stay {0, 0}, callers fail closed
    return limits;
  };

  auto is_valid_link = [&] (void **frame) noexcept {
    const auto addr = reinterpret_cast<uintptr_t> (frame);

    if (addr < sizeof (void *) * 2) {
      return false; // frame pointer is null or points below any usable stack
    }
    if ((addr & (sizeof (void *) - 1)) != 0) {
      return false; // frame pointers are always pointer aligned
    }
    const auto bounds = stack_bounds ();

    if (bounds.high == 0) {
      return false; // unknown stack range, fail closed instead of wandering
    }

    if (addr < bounds.low || addr >= bounds.high) {
      return false; // frame pointer lives outside of the thread's stack
    }
    return true;
  };
  auto next_link = [&] (void **frame) noexcept {
    const auto next = reinterpret_cast<uintptr_t> (frame[0]);

    // frame links must grow upward within a bounded distance
    if (next <= reinterpret_cast<uintptr_t> (frame) || next - reinterpret_cast<uintptr_t> (frame) > (1u << 20)) {
      return static_cast<void **> (nullptr);
    }
    return reinterpret_cast<void **> (next);
  };
    #if defined(YSTL_CXX_MSVC)
  if (depth == 0) {
    return base[0];
  }
  auto frame = reinterpret_cast<void **> (base[-1]); // saved frame pointer of the caller
    #else
  if (depth == 0) {
    return base[1];
  }
  auto frame = reinterpret_cast<void **> (base[0]);
    #endif
  if (!is_valid_link (frame)) {
    return nullptr;
  }
  for (size_t index = 1; index < depth; ++index) {
    auto next = next_link (frame);

    if (next == nullptr) {
      return nullptr; // chain ends here, deeper callers are unavailable
    }
    frame = next;
  }
  if (!is_valid_link (frame)) {
    return nullptr;
  }
  return frame[1];
  #endif
}

// memory statistics snapshot
struct MemoryStatsSnapshot {
  static constexpr size_t kSizeBuckets = 20; // covers allocation sizes up to 2^20 bytes and beyond
  static constexpr size_t kRecentLength = 32; // ring of the most recent allocations
  static constexpr size_t kMaxCallSites = 128; // tracked distinct allocation request sites
  static constexpr size_t kMaxTrackedFrames = 6; // captured stack depth: request site + callers

  // recent allocation record (diagnostics only, might be torn under threading)
  struct RecentEntry {
    uint32_t size {};
    const void *from {};
  };

  // allocation request site with its caller chain (frame 0 is the request site)
  struct CallSite {
    const void *frames[kMaxTrackedFrames] {};
    uint32_t count {};
  };

  uint64_t num_allocations {};
  uint64_t num_deallocations {};
  uint64_t num_reallocations {};
  uint64_t current_bytes {};
  uint64_t peak_bytes {};
  uint64_t total_bytes_allocated {};

  // allocation counts/bytes bucketed by size, bucket i holds allocations of (2^i, 2^(i+1)] bytes
  uint64_t bucket_allocations[kSizeBuckets] {};
  uint64_t bucket_bytes[kSizeBuckets] {};

  // last allocations in ring order, cursor counts total allocations happened
  RecentEntry recent[kRecentLength] {};
  uint32_t recent_cursor {};

  // distinct allocation request sites seen so far, with their allocation counts
  CallSite call_sites[kMaxCallSites] {};
  uint32_t call_site_overflow {}; // allocations dropped because the site table was full
};

// debug memory statistics tracker - full tracking with atomic counters
class DebugMemoryStats : public Singleton<DebugMemoryStats> {
public:
  explicit DebugMemoryStats () = default;

  Atomic<uint64_t> num_allocations {};
  Atomic<uint64_t> num_deallocations {};
  Atomic<uint64_t> num_reallocations {};
  Atomic<uint64_t> current_bytes {};
  Atomic<uint64_t> peak_bytes {};
  Atomic<uint64_t> total_bytes_allocated {};

  Atomic<uint64_t> bucket_allocations[MemoryStatsSnapshot::kSizeBuckets] {};
  Atomic<uint64_t> bucket_bytes[MemoryStatsSnapshot::kSizeBuckets] {};

  MemoryStatsSnapshot::RecentEntry recent[MemoryStatsSnapshot::kRecentLength] {};
  Atomic<uint32_t> recent_cursor {};

  // distinct allocation request sites, with atomic allocation counters (debug diagnostics only)
  struct TrackedCallSite {
    const void *frames[MemoryStatsSnapshot::kMaxTrackedFrames] {};
    Atomic<uint32_t> count {};
  };

  TrackedCallSite call_sites[MemoryStatsSnapshot::kMaxCallSites] {};
  Atomic<uint32_t> call_site_overflow {};

  MemoryStatsSnapshot get () noexcept {
    MemoryStatsSnapshot s {};

    s.num_allocations = num_allocations.load (MemoryOrder::relaxed);
    s.num_deallocations = num_deallocations.load (MemoryOrder::relaxed);
    s.num_reallocations = num_reallocations.load (MemoryOrder::relaxed);
    s.current_bytes = current_bytes.load (MemoryOrder::relaxed);
    s.peak_bytes = peak_bytes.load (MemoryOrder::relaxed);
    s.total_bytes_allocated = total_bytes_allocated.load (MemoryOrder::relaxed);
    s.recent_cursor = recent_cursor.load (MemoryOrder::relaxed);
    s.call_site_overflow = call_site_overflow.load (MemoryOrder::relaxed);

    for (size_t i = 0; i < MemoryStatsSnapshot::kSizeBuckets; ++i) {
      s.bucket_allocations[i] = bucket_allocations[i].load (MemoryOrder::relaxed);
      s.bucket_bytes[i] = bucket_bytes[i].load (MemoryOrder::relaxed);
    }

    for (size_t i = 0; i < MemoryStatsSnapshot::kRecentLength; ++i) {
      s.recent[i] = recent[i];
    }

    for (size_t i = 0; i < MemoryStatsSnapshot::kMaxCallSites; ++i) {
      for (size_t j = 0; j < MemoryStatsSnapshot::kMaxTrackedFrames; ++j) {
        s.call_sites[i].frames[j] = call_sites[i].frames[j];
      }
      s.call_sites[i].count = call_sites[i].count.load (MemoryOrder::relaxed);
    }
    return s;
  }

  void reset () noexcept {
    num_allocations.store (0, MemoryOrder::relaxed);
    num_deallocations.store (0, MemoryOrder::relaxed);
    num_reallocations.store (0, MemoryOrder::relaxed);
    current_bytes.store (0, MemoryOrder::relaxed);
    peak_bytes.store (0, MemoryOrder::relaxed);
    total_bytes_allocated.store (0, MemoryOrder::relaxed);

    for (size_t i = 0; i < MemoryStatsSnapshot::kSizeBuckets; ++i) {
      bucket_allocations[i].store (0, MemoryOrder::relaxed);
      bucket_bytes[i].store (0, MemoryOrder::relaxed);
    }
    recent_cursor.store (0, MemoryOrder::relaxed);

    for (size_t i = 0; i < MemoryStatsSnapshot::kMaxCallSites; ++i) {
      for (size_t j = 0; j < MemoryStatsSnapshot::kMaxTrackedFrames; ++j) {
        call_sites[i].frames[j] = nullptr;
      }
      call_sites[i].count.store (0, MemoryOrder::relaxed);
    }
    call_site_overflow.store (0, MemoryOrder::relaxed);
  }

  // bucket index for the size, bucket i covers sizes of (2^i, 2^(i+1)] bytes
  static size_t bucket_index (size_t size) noexcept {
    auto bucket = size_t (0);

    while (bucket + 1 < MemoryStatsSnapshot::kSizeBuckets && (size_t (1) << (bucket + 1)) <= size) {
      ++bucket;
    }
    return bucket;
  }

  // full-chain identity: sites are keyed by the whole captured stack, so the same ystl internal (e.g
  static bool same_stack (
    const void *a[MemoryStatsSnapshot::kMaxTrackedFrames], const void *b[MemoryStatsSnapshot::kMaxTrackedFrames]) noexcept {
    for (size_t index = 0; index < MemoryStatsSnapshot::kMaxTrackedFrames; ++index) {
      if (a[index] != b[index]) {
        return false;
      }
    }
    return true;
  }

  // records the request site into the site table; races here are benign (a torn concurrent write only
  // duplicates a site and fills the table sooner, counts stay approximate by design)
  template <typename... Frames> void track_site (const Frames... frames) noexcept {
    static_assert (sizeof...(Frames) <= MemoryStatsSnapshot::kMaxTrackedFrames, "too many frames for the tracked stack depth");

    const void *values[] = { frames..., nullptr };
    const void *stack[MemoryStatsSnapshot::kMaxTrackedFrames] {};

    for (size_t index = 0; index < sizeof...(Frames); ++index) {
      stack[index] = values[index];
    }

    auto site = static_cast<TrackedCallSite *> (nullptr);

    for (auto &entry : call_sites) {
      if (entry.frames[0] && same_stack (entry.frames, stack)) {
        site = &entry;
        break;
      }
      else if (!site && !entry.frames[0]) {
        site = &entry;
      }
    }

    if (site) {
      for (size_t index = 0; index < sizeof...(Frames); ++index) {
        site->frames[index] = stack[index];
      }
      ++site->count;
    }
    else {
      ++call_site_overflow;
    }
  }

  // atomic max update via cas loop, relaxed is enough for diagnostics
  static void store_max (Atomic<uint64_t> &peak, uint64_t current) noexcept {
    auto observed = peak.load (MemoryOrder::relaxed);

    while (current > observed) {
      if (peak.compare_exchange (observed, current, MemoryOrder::relaxed, MemoryOrder::relaxed)) {
        break;
      }
    }
  }

  // tracks the allocation with its captured request stack
  template <typename... Frames> void track_alloc (const size_t size, const Frames... frames) noexcept {
    const void *values[] = { frames..., nullptr };
    const auto from = values[0];

    ++num_allocations;
    current_bytes += size;
    total_bytes_allocated += size;

    store_max (peak_bytes, current_bytes.load (MemoryOrder::relaxed));
    const auto bucket = bucket_index (size);

    ++bucket_allocations[bucket];
    bucket_bytes[bucket] += size;

    // record allocation into the recent ring, fetch_add hands each thread a unique slot (torn entry
    // contents stay benign, debug diagnostics only)
    const auto slot = recent_cursor.fetch_add (1, MemoryOrder::relaxed) % MemoryStatsSnapshot::kRecentLength;

    recent[slot].size = static_cast<uint32_t> (size);
    recent[slot].from = from;

    track_site (frames...);
  }

  void track_dealloc (size_t size) noexcept {
    ++num_deallocations;
    current_bytes -= size;
  }

  template <typename... Frames> void track_realloc (size_t old_size, size_t new_size, const Frames... frames) noexcept {
    ++num_reallocations;

    if (old_size > 0 && old_size <= current_bytes.load (MemoryOrder::relaxed)) {
      current_bytes -= old_size;
    }

    current_bytes += new_size;
    total_bytes_allocated += new_size;

    store_max (peak_bytes, current_bytes.load (MemoryOrder::relaxed));
    const auto bucket = bucket_index (new_size);

    ++bucket_allocations[bucket];
    bucket_bytes[bucket] += new_size;

    // attribute the growth to its request site, like trackalloc does
    track_site (frames...);
  }
};

using MemoryStats = DebugMemoryStats;

YSTL_EXPOSE_GLOBAL_SINGLETON (MemoryStats, memstats);
#endif

namespace detail {
#if defined(YSTL_DEBUG)
YSTL_FORCE_INLINE size_t usable_size (void *memory) noexcept {
  #if defined(YSTL_WINDOWS)
  return _msize (memory);
  #elif defined(YSTL_MACOS)
  return malloc_size (memory);
  #elif defined(YSTL_LINUX) || defined(YSTL_ANDROID)
  return malloc_usable_size (memory);
  #elif defined(YSTL_BSD) || defined(YSTL_HURD) || defined(YSTL_SOLARIS) || defined(YSTL_HAIKU)
  return malloc_usable_size (memory);
  #else
  return size_t (0);
  #endif
}
#endif

template <typename T> YSTL_FORCE_INLINE T *check_alloc (void *memory, const size_t size) noexcept {
  if (!memory) {
    FixedArray<char, 384> errmsg {};
    snprintf (errmsg.data (), errmsg.capacity (), "Failed to allocate %zu bytes of memory. Closing down.", size);

    plat.abort (errmsg.data ());
  }
  return reinterpret_cast<T *> (memory);
}
}

// allocates raw memory for length objects of type t
template <typename T> YSTL_FORCE_INLINE T *allocate (const size_t length = 1) noexcept {
  static_assert (alignof (T) <= alignof (std::max_align_t), "over-aligned heap types need an aligned allocator");

  if (length > numeric_limits<size_t>::max () / sizeof (T)) [[unlikely]] {
    plat.abort ("Requested allocation size overflows. Closing down.");
  }
  auto size = length * sizeof (T);
  auto result = malloc (size);

#if defined(YSTL_DEBUG)
  // capture the stack here so the frame base stays in scope
  const auto frame_base = YSTL_MEM_FRAME_BASE;

  MemoryStats::instance ().track_alloc (size, stack_address (frame_base, 0), stack_address (frame_base, 1), stack_address (frame_base, 2),
    stack_address (frame_base, 3), stack_address (frame_base, 4), stack_address (frame_base, 5));
#endif
  return detail::check_alloc<T> (result, size);
}

// allocates zeroed memory for length objects of type t
template <typename T> YSTL_FORCE_INLINE T *allocate_zeroed (const size_t length = 1) noexcept {
  if (length > numeric_limits<size_t>::max () / sizeof (T)) [[unlikely]] {
    plat.abort ("Requested allocation size overflows. Closing down.");
  }
  auto size = length * sizeof (T);
  auto result = calloc (length, sizeof (T));

#if defined(YSTL_DEBUG)
  const auto frame_base = YSTL_MEM_FRAME_BASE;

  MemoryStats::instance ().track_alloc (size, stack_address (frame_base, 0), stack_address (frame_base, 1), stack_address (frame_base, 2),
    stack_address (frame_base, 3), stack_address (frame_base, 4), stack_address (frame_base, 5));
#endif
  return detail::check_alloc<T> (result, size);
}

// releases memory and returns nullptr
template <typename T> YSTL_FORCE_INLINE T *release (T *memory) noexcept {
  if (memory) {
#if defined(YSTL_DEBUG)
    MemoryStats::instance ().track_dealloc (detail::usable_size (memory));
#endif
    free (memory);
  }
  return nullptr;
}

// in-place constructs a single object with forwarded arguments
template <typename T, typename... Args> YSTL_FORCE_INLINE T *construct (T *memory, Args &&...args) noexcept {
  new (memory) T (ystl::forward<Args> (args)...);
  return memory;
}

// calls destructor on a single object
template <typename T> YSTL_FORCE_INLINE void destruct (T *memory) noexcept {
  memory->~T ();
}

// calls destructor and releases memory, returns nullptr for chaining
template <typename T> YSTL_FORCE_INLINE T *destruct_and_release (T *memory) noexcept {
  if (memory) {
    destruct (memory);
    return release (memory);
  }
  return nullptr;
}

// in-place constructs an array of objects with forwarded arguments
template <typename T, typename... Args> YSTL_FORCE_INLINE T *construct_array (T *memory, size_t length, Args &&...args) noexcept {
  for (size_t i = 0; i < length; ++i) {
    new (&memory[i]) T (ystl::forward<Args> (args)...);
  }
  return memory;
}

// calls destructors on an array of objects
template <typename T> YSTL_FORCE_INLINE void destruct_array (T *memory, size_t length) noexcept {
  for (size_t i = 0; i < length; ++i) {
    memory[i].~T ();
  }
}

// calls destructors on an array and releases memory, returns nullptr for chaining
template <typename T> YSTL_FORCE_INLINE T *destruct_and_release_array (T *memory, size_t length) noexcept {
  if (memory) {
    destruct_array (memory, length);
    return release (memory);
  }
  return nullptr;
}

// allocates and constructs a single object
template <typename T, typename... Args> YSTL_FORCE_INLINE T *allocate_and_construct (Args &&...args) noexcept {
  return construct<T> (allocate<T> (), ystl::forward<Args> (args)...);
}

// reallocates raw memory for trivially copyable types (can extend in-place)
template <typename T> YSTL_FORCE_INLINE T *reallocate (T *memory, const size_t length) noexcept {
  if (length == 0) [[unlikely]] {
    return release (memory);
  }

  if (length > numeric_limits<size_t>::max () / sizeof (T)) [[unlikely]] {
    plat.abort ("Requested reallocation size overflows. Closing down.");
  }
  auto size = length * sizeof (T);

#if defined(YSTL_DEBUG)
  auto old_size = memory ? detail::usable_size (memory) : size_t (0);

  // capture the request stack before realloc, same rule as in allocate()
  const auto frame_base = YSTL_MEM_FRAME_BASE;
#endif
  auto result = reinterpret_cast<T *> (realloc (memory, size));

  if (!result) {
    FixedArray<char, 384> errmsg {};
    snprintf (errmsg.data (), errmsg.capacity (), "Failed to reallocate %zu bytes of memory. Closing down.", size);

    plat.abort (errmsg.data ());
  }
#if defined(YSTL_DEBUG)
  MemoryStats::instance ().track_realloc (old_size, size, stack_address (frame_base, 0), stack_address (frame_base, 1),
    stack_address (frame_base, 2), stack_address (frame_base, 3), stack_address (frame_base, 4), stack_address (frame_base, 5));
#endif
  return result;
}

// moves elements from src to dest, destroying src elements
template <typename T> YSTL_FORCE_INLINE void transfer (T *dest, T *src, size_t length) noexcept {
  if constexpr (ystl::is_trivially_copyable_v<T>) {
    memmove (dest, src, length * sizeof (T));
  }
  else {
    for (size_t i = 0; i < length; ++i) {
      construct<T> (&dest[i], ystl::move (src[i]));
      destruct<T> (&src[i]);
    }
  }
}
}

}

// allocation debugger header pulled in here for debug builds only
#if defined(YSTL_DEBUG)
  #include <ystl/memory_debugger.h>
#endif
