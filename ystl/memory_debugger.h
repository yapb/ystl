// SPDX-License-Identifier: Unlicense

#pragma once

#include <cstddef>

#include <ystl/array.h>
#include <ystl/lambda.h>
#include <ystl/library.h>
#include <ystl/memory.h>
#include <ystl/string.h>

namespace ystl {

namespace mem {

#if defined(YSTL_DEBUG)
// allocation statistics debugger: symbol resolution plus rendering of the tracker snapshots
class MemoryDebugger final {
private:
  MemoryDebugger () = delete;

public:
  // persistent state for rate reporting between print invocations
  struct State {
    MemoryStatsSnapshot prev {};
    float prev_call_time {};
    bool has_prev_call {};
  };

private:
  // last symbolizer error shown in the report header
  static unsigned long &symbol_error () {
    static unsigned long err = 0;
    return err;
  }

  // where the symbolizer failed: 1 = library load/resolve, 2 = initialize, 3 = address resolve
  static int &symbol_error_stage () {
    static int stage = 0;
    return stage;
  }

  // formats one line and hands it to the caller's print callback (chat, log, ...)
  template <typename... Args> static void emit_line (const Lambda<void (StringRef)> &print, const char *fmt, Args &&...args) {
    String line {};

    line.assignf (fmt, ystl::forward<Args> (args)...);
    print (line);
  }

public:
  // resolves a code address to "function+0xoffset" for the memory report
  static String symbol_name (const void *addr) {
    if (!addr) {
      return "";
    }
  #if defined(YSTL_WINDOWS)
    // minimal symbol_info layout, declared locally to avoid including <dbghelp.h>
    struct MemSymbolInfo {
      unsigned long size_of_struct;
      unsigned long type_index;
      unsigned long long reserved[2];
      unsigned long index;
      unsigned long size;
      unsigned long long mod_base;
      unsigned long flags;
      unsigned long long value;
      unsigned long long address;
      unsigned long reg;
      unsigned long scope;
      unsigned long tag;
      unsigned long name_len;
      unsigned long max_name_len;
      char name[256];
    };

    using SymInitializeFn = int (WINAPI *) (void *, const char *, int);
    using SymSetOptionsFn = unsigned long (WINAPI *) (unsigned long);
    using SymFromAddrFn = int (WINAPI *) (void *, unsigned long long, unsigned long long *, MemSymbolInfo *);

    static SharedLibrary dbghelp;
    static SymInitializeFn sym_init = nullptr;
    static SymFromAddrFn sym_from_addr = nullptr;
    static bool ready = false, tried = false;

    // the outside process address is needed by symfromaddr, resolve it once
    static void *process = nullptr;

    if (!tried) {
      tried = true;

      if (dbghelp.load ("dbghelp.dll", false) && (sym_init = dbghelp.resolve<SymInitializeFn> ("SymInitialize")) != nullptr &&
          (sym_from_addr = dbghelp.resolve<SymFromAddrFn> ("SymFromAddr")) != nullptr) {
        if (auto sym_set_options = dbghelp.resolve<SymSetOptionsFn> ("SymSetOptions")) {
          constexpr unsigned long kUndName = 0x00000002; // symopt_undname
          constexpr unsigned long kDeferredLoads = 0x00000004; // symopt_deferred_loads

          sym_set_options (kUndName | kDeferredLoads);
        }
        process = GetCurrentProcess ();

        // default symbol path taken from the own module directory
        String search_path {};
        const auto self_path = SharedLibrary::path (reinterpret_cast<void *> (&MemoryDebugger::symbol_name));
        const auto sep = self_path.find_last_of ("\\/");

        if (sep != String::InvalidIndex) {
          search_path = self_path.substr (0, sep);
        }
        ready = sym_init (process, search_path.empty () ? nullptr : search_path.chars (), 1) != 0;

        if (!ready) {
          symbol_error_stage () = 2;
          symbol_error () = GetLastError ();
        }
      }
      else {
        symbol_error_stage () = 1;
        symbol_error () = GetLastError ();
      }
    }

    if (ready) {
      MemSymbolInfo sym {};

      // struct size matches the sdk layout with trailing name field
      static_assert (offsetof (MemSymbolInfo, name) == 84, "MemSymbolInfo must match SYMBOL_INFO layout");
      sym.size_of_struct = 88;
      sym.max_name_len = sizeof (sym.name) - 1;

      unsigned long long displacement = 0;

      if (sym_from_addr (process, reinterpret_cast<unsigned long long> (addr), &displacement, &sym) != 0 && sym.name[0]) {
        String result {};

        result.assignf ("%s+0x%llx", sym.name, displacement);
        return result;
      }
      symbol_error_stage () = 3;
      symbol_error () = GetLastError ();
    }
    return "";
  #elif defined(YSTL_POSIX)
    Dl_info dli {};

    ystl::memzero (&dli, sizeof (dli));

    if (dladdr (const_cast<void *> (addr), &dli) && dli.dli_sname) {
      String result {};

      result.assignf (
        "%s+0x%zx", dli.dli_sname, static_cast<size_t> (reinterpret_cast<uintptr_t> (addr) - reinterpret_cast<uintptr_t> (dli.dli_saddr)));
      return result;
    }
    return "";
  #else
    return "";
  #endif
  }

  // renders the full allocation statistics report; printing is delegated to the caller (e.g
  static void print (State &state, float call_time, const Lambda<void (StringRef)> &print) {
    auto stats = memstats.get ();

    emit_line (print, " Memory Statistics ");
    emit_line (print, "  Called at:       %.2f (server time)", call_time);
    emit_line (print, "  Allocations:     %llu", stats.num_allocations);
    emit_line (print, "  Deallocations:   %llu", stats.num_deallocations);
    emit_line (print, "  Reallocations:   %llu", stats.num_reallocations);
    emit_line (print, "  Current usage:   %llu bytes (%.2f KB)", stats.current_bytes, static_cast<float> (stats.current_bytes / 1024.0));
    emit_line (print, "  Peak usage:      %llu bytes (%.2f KB)", stats.peak_bytes, static_cast<float> (stats.peak_bytes / 1024.0));
    emit_line (
      print, "  Total allocated: %llu bytes (%.2f KB)", stats.total_bytes_allocated, static_cast<float> (stats.total_bytes_allocated / 1024.0));

    if (state.has_prev_call) {
      const auto elapsed = call_time - state.prev_call_time;

      // avoid division by zero when called twice within the same frame
      if (elapsed > 0.0f) {
        const auto &prev = state.prev;

        // stats might be reset between calls, clamp deltas to zero in that case
        const auto allocs_delta = stats.num_allocations > prev.num_allocations ? stats.num_allocations - prev.num_allocations : 0;
        const auto deallocs_delta = stats.num_deallocations > prev.num_deallocations ? stats.num_deallocations - prev.num_deallocations : 0;
        const auto reallocs_delta = stats.num_reallocations > prev.num_reallocations ? stats.num_reallocations - prev.num_reallocations : 0;

        const auto seconds = static_cast<float> (elapsed);

        emit_line (print, " Per Second (previous call was %.2f sec ago) ", seconds);
        emit_line (print, "  Allocations:     %.2f", static_cast<float> (allocs_delta) / seconds);
        emit_line (print, "  Deallocations:   %.2f", static_cast<float> (deallocs_delta) / seconds);
        emit_line (print, "  Reallocations:   %.2f", static_cast<float> (reallocs_delta) / seconds);

        // size-class breakdown of allocations happened since the last call, helps to find allocation sources
        constexpr auto kSizeBuckets = MemoryStatsSnapshot::kSizeBuckets;

        struct SizeRate {
          double count {};
          double bytes {};
        };

        SizeRate rates[kSizeBuckets] = {};

        for (size_t i = 0; i < kSizeBuckets; ++i) {
          const auto bucket_allocs =
            stats.bucket_allocations[i] > prev.bucket_allocations[i] ? stats.bucket_allocations[i] - prev.bucket_allocations[i] : 0;
          const auto bucket_bytes = stats.bucket_bytes[i] > prev.bucket_bytes[i] ? stats.bucket_bytes[i] - prev.bucket_bytes[i] : 0;

          rates[i].count = static_cast<double> (bucket_allocs) / elapsed;
          rates[i].bytes = static_cast<double> (bucket_bytes) / elapsed;
        }

        // order non-empty buckets by allocation count, descending (tiny insertion sort)
        size_t order[kSizeBuckets] = {};
        size_t num_rate_buckets = 0;

        for (size_t i = 0; i < kSizeBuckets; ++i) {
          if (rates[i].count >= 0.01) {
            order[num_rate_buckets++] = i;
          }
        }

        for (size_t i = 1; i < num_rate_buckets; ++i) {
          const auto value = order[i];
          auto j = i;

          while (j > 0 && rates[order[j - 1]].count < rates[value].count) {
            order[j] = order[j - 1];
            --j;
          }
          order[j] = value;
        }

        if (num_rate_buckets > 0) {
          emit_line (print, " Periodic by Size ");
        }

        for (size_t i = 0; i < num_rate_buckets; ++i) {
          const auto bucket = order[i];
          const auto low = 1u << bucket;
          const auto high = (1u << (bucket + 1)) - 1;

          if (bucket + 1 == kSizeBuckets) {
            emit_line (print, "  %u+ bytes : %10.2f/s (%10.2f KB/s)", low, rates[bucket].count, rates[bucket].bytes / 1024.0);
          }
          else {
            emit_line (print, "  %u-%u bytes : %10.2f/s (%10.2f KB/s)", low, high, rates[bucket].count, rates[bucket].bytes / 1024.0);
          }
        }

        // allocation rates grouped by request site as module offsets
        constexpr auto kMaxCallSites = MemoryStatsSnapshot::kMaxCallSites;

        struct SiteRate {
          const void *frames[MemoryStatsSnapshot::kMaxTrackedFrames] {};
          double rate {};
          uint32_t total {};
        };

        SiteRate sites[kMaxCallSites] = {};
        size_t num_sites = 0;

        for (size_t i = 0; i < kMaxCallSites; ++i) {
          const auto &site = stats.call_sites[i];

          if (!site.frames[0] || !site.count) {
            continue;
          }
          auto delta = static_cast<double> (site.count);

          // site counts are cumulative, subtract the previous snapshot
          for (size_t j = 0; j < kMaxCallSites; ++j) {
            bool same = prev.call_sites[j].frames[0] != nullptr;

            for (size_t k = 0; same && k < MemoryStatsSnapshot::kMaxTrackedFrames; ++k) {
              same = prev.call_sites[j].frames[k] == site.frames[k];
            }

            if (same) {
              delta = site.count > prev.call_sites[j].count ? static_cast<double> (site.count - prev.call_sites[j].count) : 0.0;
              break;
            }
          }
          for (size_t j = 0; j < MemoryStatsSnapshot::kMaxTrackedFrames; ++j) {
            sites[num_sites].frames[j] = site.frames[j];
          }
          sites[num_sites].rate = delta / elapsed;
          sites[num_sites].total = site.count;
          ++num_sites;
        }

        for (size_t i = 1; i < num_sites; ++i) {
          const auto value = sites[i];
          auto j = i;

          while (j > 0 && sites[j - 1].rate < value.rate) {
            sites[j] = sites[j - 1];
            --j;
          }
          sites[j] = value;
        }

        // resolve the base address of own module, so call sites can be printed as module relative
        static const void *self_base = nullptr;

        if (!self_base) {
  #if defined(YSTL_WINDOWS)
          MEMORY_BASIC_INFORMATION mbi {};

          if (VirtualQuery (&self_base, &mbi, sizeof (mbi))) {
            self_base = mbi.AllocationBase;
          }
  #elif defined(YSTL_POSIX)
          Dl_info dli {};

          if (dladdr (&self_base, &dli)) {
            self_base = dli.dli_fbase;
          }
  #endif
        }

        if (num_sites > 0) {
          // probe symbols once to label output as named or raw rvas
          static int sym_state = -1;

          if (sym_state < 0) {
            sym_state = symbol_name (reinterpret_cast<const void *> (&MemoryDebugger::symbol_name)).empty () ? 0 : 1;
          }

          if (sym_state) {
            emit_line (print, " Periodic by Call Site (symbols on) ");
          }
          else {
            emit_line (print, " Periodic by Call Site (raw rvas, sym stage %d err %lu) ", symbol_error_stage (), symbol_error ());
          }
        }

        for (size_t i = 0; i < num_sites && i < 10; ++i) {
          if (self_base) {
            const auto rva = reinterpret_cast<size_t> (sites[i].frames[0]) - reinterpret_cast<size_t> (self_base);
            const auto sym_name = symbol_name (sites[i].frames[0]);

            // collect the caller chain as module relative addresses
            SmallArray<unsigned, 8> chain {};

            for (size_t j = 1; j < MemoryStatsSnapshot::kMaxTrackedFrames; ++j) {
              if (sites[i].frames[j]) {
                chain.push (static_cast<unsigned> (reinterpret_cast<size_t> (sites[i].frames[j]) - reinterpret_cast<size_t> (self_base)));
              }
            }

            // print the full attribution chain: allocation site <- its callers up the stack
            switch (chain.size ()) {
            case 5:
              emit_line (print, "  %10.2f/s  rva 0x%08x <- 0x%08x <- 0x%08x <- 0x%08x <- 0x%08x <- 0x%08x  (total %u)", sites[i].rate,
                static_cast<unsigned> (rva), chain[0], chain[1], chain[2], chain[3], chain[4], sites[i].total);
              break;
            case 4:
              emit_line (print, "  %10.2f/s  rva 0x%08x <- 0x%08x <- 0x%08x <- 0x%08x <- 0x%08x  (total %u)", sites[i].rate,
                static_cast<unsigned> (rva), chain[0], chain[1], chain[2], chain[3], sites[i].total);
              break;
            case 3:
              emit_line (print, "  %10.2f/s  rva 0x%08x <- 0x%08x <- 0x%08x <- 0x%08x  (total %u)", sites[i].rate, static_cast<unsigned> (rva),
                chain[0], chain[1], chain[2], sites[i].total);
              break;
            case 2:
              emit_line (print, "  %10.2f/s  rva 0x%08x <- 0x%08x <- 0x%08x  (total %u)", sites[i].rate, static_cast<unsigned> (rva), chain[0],
                chain[1], sites[i].total);
              break;
            case 1:
              emit_line (
                print, "  %10.2f/s  rva 0x%08x <- 0x%08x  (total %u)", sites[i].rate, static_cast<unsigned> (rva), chain[0], sites[i].total);
              break;
            default:
              emit_line (print, "  %10.2f/s  rva 0x%08x  (total %u)", sites[i].rate, static_cast<unsigned> (rva), sites[i].total);
              break;
            }

            // full attribution chain with symbols: site <- callers
            String chain_text {};

            auto append_link = [&chain_text] (const String &name) {
              if (!chain_text.empty ()) {
                chain_text += " <- ";
              }
              chain_text += name.chars ();
            };

            if (!sym_name.empty ()) {
              append_link (sym_name);
            }

            for (size_t c = 0; c < chain.size (); ++c) {
              const auto caller_addr = reinterpret_cast<const void *> (reinterpret_cast<size_t> (self_base) + chain[c]);
              const auto caller_name = symbol_name (caller_addr);

              if (caller_name.empty ()) {
                String rva_text {};

                rva_text.assignf ("rva 0x%08x", chain[c]);
                append_link (rva_text);
              }
              else {
                append_link (caller_name);
              }
            }

            if (!chain_text.empty ()) {
              emit_line (print, "     ^ %s", chain_text.chars ());
            }
          }
          else {
            emit_line (print, "  %10.2f/s  abs 0x%p  (total %u)", sites[i].rate, sites[i].frames[0], sites[i].total);
          }
        }

        const auto overflow = stats.call_site_overflow > prev.call_site_overflow ? stats.call_site_overflow - prev.call_site_overflow : 0;

        if (overflow > 0) {
          emit_line (print, "  Note: %u allocations from untracked sites (table full).", overflow);
        }
      }
    }
    state.prev = stats;
    state.prev_call_time = call_time;
    state.has_prev_call = true;

    // last allocations (newest first), addresses can be resolved against a linker map file
    emit_line (print, " Recent Allocations (newest first) ");

    constexpr auto kRecentLength = MemoryStatsSnapshot::kRecentLength;
    const auto total_allocs = static_cast<uint64_t> (stats.recent_cursor);

    for (size_t i = 0; i < kRecentLength; ++i) {
      if (total_allocs < i + 1) {
        break;
      }
      const auto slot = static_cast<size_t> ((total_allocs - 1 - i) % kRecentLength);
      const auto &entry = stats.recent[slot];

      if (!entry.size) {
        continue;
      }
      const auto recent_sym = symbol_name (entry.from);

      if (recent_sym.empty ()) {
        emit_line (print, "  %7u bytes  from  0x%p", entry.size, entry.from);
      }
      else {
        emit_line (print, "  %7u bytes  from  0x%p (%s)", entry.size, entry.from, recent_sym.chars ());
      }
    }
  }
};

#endif // defined(ystl_debug)

} // namespace mem

}
