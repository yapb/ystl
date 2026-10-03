#pragma once

// ystl-based test & benchmark framework (single header, no std:: usage)
//
// layers:
//   TestRegistry      static registration of cases (TEST_CASE)
//   ITestReporter     runner -> output interface (console / junit)
//   ystl::test::runMain shared argv parser + runner entry (console|junit)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <ystl/ystl.h>

#if !defined(YSTL_WINDOWS)
  #include <sys/ioctl.h>
  #include <sys/wait.h>
#endif

// --- test registration ---

struct TestCase {
  const char *name;
  ystl::Lambda<void ()> func;
  const char *file;
  int line;
};

// all registered cases; a small class instead of a bare global blob so the
// runner and reporters depend on an interface, not on storage
class TestRegistry {
public:
  void add (const char *name, ystl::Lambda<void ()> fn, const char *file, int line) {
    cases_.push (TestCase { name, ystl::move (fn), file, line });
  }

  ystl::Array<TestCase> &cases () {
    return cases_;
  }
  const ystl::Array<TestCase> &cases () const {
    return cases_;
  }

private:
  ystl::Array<TestCase> cases_;
};

inline TestRegistry &registry () {
  static TestRegistry r;
  return r;
}

struct TestRegistrar {
  TestRegistrar (const char *name, ystl::Lambda<void ()> fn, const char *file, int line) {
    registry ().add (name, ystl::move (fn), file, line);
  }
};

// --- section support ---

struct SectionTracker {
  ystl::Array<const char *> entered;
  const char *last_section { nullptr };

  void reset () {
    entered.clear ();
    last_section = nullptr;
  }

  bool should_run (const char *name) {
    last_section = name;

    for (auto *e : entered) {
      if (strcmp (e, name) == 0)
        return false;
    }
    entered.push (name);
    return true;
  }
};

extern thread_local SectionTracker g_sections;

// --- result tracking ---

struct TestResults {
  int passed { 0 };
  int failed { 0 };
  int assertions { 0 };
  const char *current_test { nullptr };
  bool current_failed { false };

  // records a failed assertion (and forwards it to the active reporter)
  void fail (const char *expr, const char *file, int line, const char *msg = "");
  void pass () {
    assertions++;
    passed++;
  }
};

extern thread_local TestResults g_results;

// --- reporters: the runner talks to ITestReporter only, so console, junit
// and future backends stay pluggable ---

namespace ystl::test {

struct Failure {
  const char *expr {};
  const char *message {};
  const char *file {};
  int line {};
};

struct CaseResult {
  const char *name {};
  double ms {};
  int assertions {};
  bool failed {};
};

struct SuiteResult {
  int total { 0 };
  int passed { 0 };
  int failed { 0 };
  int assertions { 0 };
  double wall_seconds {};
  int benchmarks { 0 };
  double benchmarked_ns {};
  char benchmark_time[32] { "n/a" };
  ystl::Array<const char *> failed_names;
};

class ITestReporter {
public:
  virtual ~ITestReporter () = default;

  virtual void on_suite_start (int total, int name_width) {
    (void)total;
    (void)name_width;
  }
  virtual void on_case_start (const TestCase &) {}
  virtual void on_failure (const Failure &) {}
  virtual void on_case_end (const TestCase &, const CaseResult &) {}
  virtual void on_suite_end (const SuiteResult &) {}

  // reporters that own the console suppress the inline benchmark tables
  virtual bool console_output () const {
    return true;
  }
};

// human console: running index, colored status, aligned names, a per-group
// breakdown, failure locations and the slowest cases at the end. Colors
// auto-enable on a terminal (NO_COLOR honored) and can be forced with
// --color=always|never; --quiet prints only failures plus the summary.
class ConsoleReporter final : public ITestReporter {
public:
  enum class Color {
    Auto,
    Always,
    Never
  };

  void set_color (Color color) {
    color_ = color;
  }
  void set_quiet (bool quiet) {
    quiet_ = quiet;
  }
  void set_brief (bool brief) {
    brief_ = brief;
  }
  // parent-orchestrated runs: the child prints its one line at the global
  // position but no header/summary of its own
  void set_position (int index, int total) {
    position_index_ = index;
    position_total_ = total;
  }

  bool console_output () const override {
    return true;
  }

  void on_suite_start (int total, int name_width) override {
    total_ = position_total_ > 0 ? position_total_ : total;
    started_ = position_index_ > 0 ? position_index_ - 1 : 0;
    (void)name_width;

    results_.clear ();
    failed_.clear ();
    groups_.clear ();
    pending_.clear ();

    use_color_ = resolve_color ();
    display_width_ = terminal_width ();

    // index + name + space + status + time + asserts must fit the window,
    // and the last cell stays empty (cmd wraps when it is written)
    const int index_field = number_width (total_) * 2 + 1;
    const int fixed = 2 + index_field + 2 + 1 + kStatusWidth + kTailWidth;

    name_col_ = display_width_ - 1 - fixed;

    if (name_col_ < 16) {
      name_col_ = 16;
    }

    if (brief_) {
      return;
    }
    set_color (kCyan);
    printf ("\n  running %d %s\n", total_, total_ == 1 ? "case" : "cases");
    reset_color ();
  }

  void on_case_start (const TestCase &tc) override {
    started_++;
    pending_.clear ();

    if (quiet_) {
      return;
    }

    printf ("  ");
    set_color (kDim);
    printf ("%*d/%-*d", number_width (total_), started_, number_width (total_), total_);
    reset_color ();
    printf ("  ");
    print_name_column (tc.name);
    putchar (' ');
    fflush (stdout);
  }

  void on_failure (const Failure &f) override {
    pending_.push (f);
  }

  void on_case_end (const TestCase &tc, const CaseResult &r) override {
    record_group (tc.name, r.failed);
    results_.push (r);

    if (r.failed) {
      Failed entry {};
      entry.name = tc.name;
      entry.file = pending_.size () > 0 ? pending_[0].file : "";
      entry.line = pending_.size () > 0 ? pending_[0].line : 0;
      failed_.push (entry);
    }

    if (!quiet_) {
      set_color (r.failed ? kRed : kGreen);
      printf ("%*s", kStatusWidth, r.failed ? "FAIL" : "OK");
      reset_color ();
      set_color (kDim);
      printf (" %6.1f ms %4d a\n", r.ms, r.assertions);
      reset_color ();
    }
    else if (r.failed) {
      set_color (kRed);
      printf ("  FAIL  %s\n", tc.name);
      reset_color ();
    }

    if (r.failed) {
      for (const auto &f : pending_) {
        printf ("         ");
        set_color (kRed);
        printf ("%s\n", f.expr);
        reset_color ();
        printf ("         ");
        set_color (kDim);
        printf ("at ");
        print_short_path (f.file);
        printf (":%d", f.line);
        reset_color ();

        if (f.message && f.message[0]) {
          printf (" (%s)", f.message);
        }
        printf ("\n");
      }
    }
    fflush (stdout);
  }

  void on_suite_end (const SuiteResult &s) override {
    if (brief_) {
      return;
    }
    print_summary (s);
  }

private:
  struct Group {
    ystl::String name {};
    int total { 0 };
    int failed { 0 };
  };

  struct Failed {
    const char *name {};
    const char *file {};
    int line { 0 };
  };

  static constexpr const char *kReset = "\x1b[0m";
  static constexpr const char *kDim = "\x1b[2m";
  static constexpr const char *kRed = "\x1b[31m";
  static constexpr const char *kGreen = "\x1b[32m";
  static constexpr const char *kCyan = "\x1b[36m";

  static constexpr int kStatusWidth = 4;
  static constexpr int kTailWidth = 17; // " %6.1f ms %4d a"

  void set_color (const char *code) {
    if (use_color_) {
      fputs (code, stdout);
    }
  }
  void reset_color () {
    set_color (kReset);
  }

  // name padded with dots to name_col_; longer names get an ellipsis
  void print_name_column (const char *name) {
    const int len = static_cast<int> (strlen (name));

    if (len <= name_col_) {
      fputs (name, stdout);
      set_color (kDim);

      for (int i = len; i < name_col_; ++i) {
        putchar ('.');
      }
      reset_color ();
      return;
    }
    const int keep = name_col_ > 3 ? name_col_ - 3 : 0;

    for (int i = 0; i < keep; ++i) {
      putchar (name[i]);
    }
    fputs ("...", stdout);
  }

  // print at most max chars, tail replaced with an ellipsis
  static void print_truncated (const char *text, int max) {
    const int len = static_cast<int> (strlen (text));

    if (len <= max || max <= 3) {
      fputs (text, stdout);
      return;
    }
    for (int i = 0; i < max - 3; ++i) {
      putchar (text[i]);
    }
    fputs ("...", stdout);
  }

  // keep only the last two path components so failures fit the window
  static void print_short_path (const char *file) {
    int last = -1;
    int prev = -1;

    for (int i = 0; file && file[i]; ++i) {
      if (file[i] == '/' || file[i] == '\\') {
        prev = last;
        last = i;
      }
    }
    fputs (file + (prev >= 0 ? prev + 1 : 0), stdout);
  }

  static int number_width (int value) {
    int width = 1;

    while (value >= 10) {
      value /= 10;
      width++;
    }
    return width;
  }

  static bool stdout_is_tty () {
#if defined(YSTL_WINDOWS)
    return _isatty (_fileno (stdout)) != 0;
#else
    return isatty (fileno (stdout)) != 0;
#endif
  }

  static int terminal_width () {
#if defined(YSTL_WINDOWS)
    CONSOLE_SCREEN_BUFFER_INFO info {};

    if (GetConsoleScreenBufferInfo (GetStdHandle (STD_OUTPUT_HANDLE), &info)) {
      const int width = info.srWindow.Right - info.srWindow.Left + 1;

      if (width > 0) {
        return width;
      }
    }
#else
    struct winsize size {};

    if (ioctl (STDOUT_FILENO, TIOCGWINSZ, &size) == 0 && size.ws_col > 0) {
      return size.ws_col;
    }
#endif
    const char *columns = getenv ("COLUMNS");

    if (columns && atoi (columns) > 0) {
      return atoi (columns);
    }
    return 80;
  }

  static void enable_virtual_terminal () {
#if defined(YSTL_WINDOWS)
    const auto handle = GetStdHandle (STD_OUTPUT_HANDLE);

    if (handle && handle != INVALID_HANDLE_VALUE) {
      DWORD mode = 0;

      if (GetConsoleMode (handle, &mode)) {
        SetConsoleMode (handle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
      }
    }
#endif
  }

  bool resolve_color () const {
    if (color_ == Color::Always) {
      return true;
    }
    if (color_ == Color::Never || getenv ("NO_COLOR") != nullptr || !stdout_is_tty ()) {
      return false;
    }
    enable_virtual_terminal ();
    return true;
  }

  static ystl::String group_of (const char *name) {
    ystl::String out {};
    const char *slash = strchr (name, '/');

    // area/name: group by what the case actually covers, not the area
    // prefix (tier0/weapon_data -> weapon_data)
    if (slash) {
      for (const char *p = slash + 1; *p; ++p) {
        if (*p == ' ' && p[1] == '[') {
          break;
        }
        out += *p;
      }
      return out;
    }

    // flat names are tagged: use the first [tag] as the group
    const char *open = strchr (name, '[');

    if (open) {
      const char *close = strchr (open, ']');

      if (close && close > open + 1) {
        for (const char *p = open + 1; p < close; ++p) {
          out += *p;
        }
        return out;
      }
    }
    out += "tests";
    return out;
  }

  void record_group (const char *name, bool failed) {
    auto key = group_of (name);

    for (auto &g : groups_) {
      if (g.name == key) {
        g.total++;
        g.failed += failed ? 1 : 0;
        return;
      }
    }
    Group group {};
    group.name = ystl::move (key);
    group.total = 1;
    group.failed = failed ? 1 : 0;
    groups_.push (ystl::move (group));
  }

  void print_summary (const SuiteResult &s) {
    printf ("\n  ");
    set_color (kCyan);
    printf ("summary");
    reset_color ();
    printf ("  %d case%s | ", s.total, s.total == 1 ? "" : "s");

    set_color (kGreen);
    printf ("%d passed", s.passed);
    reset_color ();

    if (s.failed > 0) {
      printf (" | ");
      set_color (kRed);
      printf ("%d failed", s.failed);
      reset_color ();
    }
    printf (" | %d assertions | %.2f s\n", s.assertions, s.wall_seconds);

    if (s.benchmarks > 0) {
      printf ("  ");
      set_color (kCyan);
      printf ("benchmarks");
      reset_color ();
      printf ("  %d | %s\n", s.benchmarks, s.benchmark_time);
    }

    // per-group overview: all groups when few, otherwise only failures
    if (groups_.size () > 0) {
      const bool show_all = groups_.size () <= 10;
      bool printed = false;

      for (const auto &g : groups_) {
        if (!show_all && g.failed == 0) {
          continue;
        }
        if (!printed) {
          printf ("\n  ");
          set_color (kCyan);
          printf ("groups");
          reset_color ();
          printf ("\n");
          printed = true;
        }
        printf ("    %-16s %4d/%d", g.name.chars (), g.total - g.failed, g.total);

        if (g.failed > 0) {
          set_color (kRed);
          printf (" %d failed", g.failed);
          reset_color ();
        }
        printf ("\n");
      }

      if (!printed && !show_all) {
        printf ("\n  ");
        set_color (kDim);
        printf ("all %d groups passed", static_cast<int> (groups_.size ()));
        reset_color ();
        printf ("\n");
      }
    }

    // failures with their first source location (short relative path)
    if (failed_.size () > 0) {
      printf ("\n  ");
      set_color (kRed);
      printf ("failed");
      reset_color ();
      printf ("\n");

      for (const auto &f : failed_) {
        printf ("    ");
        set_color (kRed);
        print_truncated (f.name, display_width_ - 4);
        reset_color ();
        printf ("\n      at ");
        set_color (kDim);
        print_short_path (f.file);
        printf (":%d", f.line);
        reset_color ();
        printf ("\n");
      }
    }

    // benchmark binaries already print per-op tables; skip the slowest list
    if (s.benchmarks == 0) {
      print_slowest ();
    }
  }

  void print_slowest () {
    const int wanted = results_.size () < 5 ? static_cast<int> (results_.size ()) : 5;

    if (wanted <= 0 || total_ <= 1) {
      return;
    }

    ystl::Array<int> picked {};

    for (int k = 0; k < wanted; ++k) {
      int best = -1;
      double best_ms = -1.0;

      for (int i = 0; i < static_cast<int> (results_.size ()); ++i) {
        bool skip = false;

        for (int p : picked) {
          if (p == i) {
            skip = true;
            break;
          }
        }
        if (skip || results_[i].ms <= best_ms) {
          continue;
        }
        best_ms = results_[i].ms;
        best = i;
      }

      if (best < 0) {
        break;
      }
      picked.push (best);

      if (k == 0) {
        printf ("\n  ");
        set_color (kCyan);
        printf ("slowest");
        reset_color ();
        printf ("\n");
      }
      printf ("    ");
      set_color (kDim);
      printf ("%7.2f ms ", best_ms);
      reset_color ();
      print_truncated (results_[best].name, display_width_ - 14);
      printf ("\n");
    }
  }

private:
  bool quiet_ { false };
  bool brief_ { false };
  bool use_color_ { false };
  Color color_ { Color::Auto };
  int total_ { 0 };
  int display_width_ { 80 };
  int name_col_ { 40 };
  int started_ { 0 };
  int position_index_ { 0 };
  int position_total_ { 0 };
  ystl::Array<Failure> pending_ {};
  ystl::Array<CaseResult> results_ {};
  ystl::Array<Failed> failed_ {};
  ystl::Array<Group> groups_ {};
};

// junit xml for CI test reporting (written once at suite end)
class JUnitReporter final : public ITestReporter {
public:
  explicit JUnitReporter (const char *path) : path_ (path) {}

  bool console_output () const override {
    return false;
  }

  void on_suite_start (int total, int) override {
    total_ = total;
    body_.clear ();
  }

  void on_case_start (const TestCase &) override {
    failures_.clear ();
  }

  void on_failure (const Failure &f) override {
    failures_.push (f);
  }

  void on_case_end (const TestCase &tc, const CaseResult &r) override {
    char buf[64] {};

    body_ += "  <testcase name=\"";
    body_ += escape (tc.name);
    body_ += "\" classname=\"";
    body_ += escape (tc.file);
    body_ += "\" time=\"";
    snprintf (buf, sizeof (buf), "%.6f", r.ms / 1000.0);
    body_ += buf;

    if (!r.failed && failures_.size () == 0) {
      body_ += "\"/>\n";
      return;
    }
    body_ += "\">\n";

    for (const auto &f : failures_) {
      body_ += "    <failure message=\"";
      body_ += escape (f.expr);
      body_ += "\">";
      body_ += escape (f.file);
      snprintf (buf, sizeof (buf), ":%d", f.line);
      body_ += buf;

      if (f.message && f.message[0]) {
        body_ += " ";
        body_ += escape (f.message);
      }
      body_ += "</failure>\n";
    }
    body_ += "  </testcase>\n";
  }

  void on_suite_end (const SuiteResult &s) override {
    char buf[256] {};
    ystl::String xml {};

    snprintf (buf, sizeof (buf),
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      "<testsuite name=\"tests\" tests=\"%d\" failures=\"%d\" errors=\"0\" skipped=\"0\" time=\"%.6f\">\n",
      s.total, s.failed, s.wall_seconds);
    xml += buf;
    xml += body_;
    xml += "</testsuite>\n";

    if (FILE *fp = fopen (path_.chars (), "wb")) {
      fwrite (xml.chars (), 1, xml.size (), fp);
      fclose (fp);
      printf ("junit report written to %s\n", path_.chars ());
    }
    else {
      printf ("unable to write junit report to %s\n", path_.chars ());
    }
  }

private:
  // xml attribute/text escaping
  static ystl::String escape (const char *s) {
    ystl::String out {};

    for (; s && *s; ++s) {
      switch (*s) {
      case '&':
        out += "&amp;";
        break;
      case '<':
        out += "&lt;";
        break;
      case '>':
        out += "&gt;";
        break;
      case '"':
        out += "&quot;";
        break;
      case '\'':
        out += "&apos;";
        break;
      default:
        out += *s;
        break;
      }
    }
    return out;
  }

private:
  ystl::String path_ {};
  ystl::String body_ {};
  ystl::Array<Failure> failures_ {};
  int total_ { 0 };
};

inline ITestReporter *&reporter_slot () {
  static ITestReporter *reporter = nullptr;
  return reporter;
}

inline ConsoleReporter &console_reporter () {
  static ConsoleReporter reporter;
  return reporter;
}

inline ITestReporter &active_reporter () {
  return reporter_slot () ? *reporter_slot () : console_reporter ();
}

inline void set_reporter (ITestReporter *reporter) {
  reporter_slot () = reporter;
}

// last finished suite, and an optional file the orchestrator uses to
// collect per-child counts (fork-per-case mode)
inline SuiteResult &last_suite () {
  static SuiteResult suite;
  return suite;
}

inline const char *&result_path_storage () {
  static const char *value = "";
  return value;
}

} // namespace ystl::test

inline void TestResults::fail (const char *expr, const char *file, int line, const char *msg) {
  assertions++;
  current_failed = true;
  failed++;

  ystl::test::active_reporter ().on_failure (ystl::test::Failure { expr, msg, file, line });
}

// --- approx ---

struct Approx {
  double value;
  double eps { 1e-8 };

  explicit Approx (double v) : value (v) {}
  explicit Approx (float v) : value (static_cast<double> (v)) {}
  Approx &epsilon (double e) {
    eps = e;
    return *this;
  }
  Approx &epsilon (float e) {
    eps = static_cast<double> (e);
    return *this;
  }
  Approx &margin (double m) {
    eps = m;
    return *this;
  }
  Approx &margin (float m) {
    eps = static_cast<double> (m);
    return *this;
  }

  bool check_eq (double other) const {
    return ystl::abs (other - value) <= eps * ystl::abs (value);
  }
  bool check_ne (double other) const {
    return !check_eq (other);
  }
  bool check_gt (double other) const {
    return other > value;
  }
  bool check_lt (double other) const {
    return other < value;
  }
  bool check_ge (double other) const {
    return other >= value;
  }
  bool check_le (double other) const {
    return other <= value;
  }

  friend bool operator== (double lhs, const Approx &rhs) {
    return rhs.check_eq (lhs);
  }
  friend bool operator== (const Approx &lhs, double rhs) {
    return lhs.check_eq (rhs);
  }
  friend bool operator!= (double lhs, const Approx &rhs) {
    return rhs.check_ne (lhs);
  }
  friend bool operator!= (const Approx &lhs, double rhs) {
    return lhs.check_ne (rhs);
  }
  friend bool operator> (double lhs, const Approx &rhs) {
    return lhs > rhs.value;
  }
  friend bool operator> (const Approx &lhs, double rhs) {
    return lhs.check_gt (rhs);
  }
  friend bool operator< (double lhs, const Approx &rhs) {
    return lhs < rhs.value;
  }
  friend bool operator< (const Approx &lhs, double rhs) {
    return lhs.check_lt (rhs);
  }
  friend bool operator>= (double lhs, const Approx &rhs) {
    return lhs >= rhs.value;
  }
  friend bool operator>= (const Approx &lhs, double rhs) {
    return lhs.check_ge (rhs);
  }
  friend bool operator<= (double lhs, const Approx &rhs) {
    return lhs <= rhs.value;
  }
  friend bool operator<= (const Approx &lhs, double rhs) {
    return lhs.check_le (rhs);
  }

  // float overloads
  friend bool operator== (float lhs, const Approx &rhs) {
    return rhs.check_eq (static_cast<double> (lhs));
  }
  friend bool operator== (const Approx &lhs, float rhs) {
    return lhs.check_eq (static_cast<double> (rhs));
  }
  friend bool operator!= (float lhs, const Approx &rhs) {
    return rhs.check_ne (static_cast<double> (lhs));
  }
  friend bool operator!= (const Approx &lhs, float rhs) {
    return lhs.check_ne (static_cast<double> (rhs));
  }
};

// --- benchmark framework ---

namespace ystl {
namespace benchmark {

// record of a single benchmark measurement
struct Result {
  const char *name {};
  const char *section {}; // owning section (), if any
  int runs { 0 };
  double ns_per_op {};
  double total_ns {};

  // display name with the owning section as prefix
  void display_name (char *out, size_t size) const {
    if (section) {
      snprintf (out, size, "%s/%s", section, name);
    }
    else {
      snprintf (out, size, "%s", name);
    }
  }
};

// collects results from all executed benchmarks
struct Reporter {
  ystl::Array<Result> results;

  static Reporter &instance () {
    static Reporter r;
    return r;
  }
};

// prevents the compiler from optimizing away benchmarked values
template <typename T> void deoptimize_value (T const &value) {
#if defined(YSTL_CXX_GCC) || defined(YSTL_CXX_CLANG)
  asm volatile ("" : : "r,m"(value) : "memory");
#else
  // msvc: force a volatile load of the value through its own storage
  (void)(*reinterpret_cast<const T volatile *> (&value));
#endif
}

// untimed warmup batches before every measurement (configurable via --warmup=n)
inline int &default_warmup () {
  static int warmup = 3;
  return warmup;
}

// timed repetitions per benchmark, the minimum is reported (configurable via --repeat=n)
inline int &default_repeats () {
  static int repeats = 5;
  return repeats;
}

// measures a batch of runs, reports per-operation and total time
class Chronometer {
  int runs_ { 0 };
  double total_ns_ {};

  template <typename F> void warmup (F &&fn, int batches) {
    for (int w = 0; w < batches; ++w) {
      fn ();
    }
  }

public:
  explicit Chronometer (int runs) : runs_ (runs) {}

  int runs () const {
    return runs_;
  }

  template <typename F> void measure (F &&fn) {
    warmup (fn, default_warmup ());

    double best = -1.0;

    for (int r = 0; r < default_repeats (); ++r) {
      const Stopwatch sw;

      for (int i = 0; i < runs_; ++i) {
        fn ();
      }
      const double total = sw.elapsed_ns ();

      if (best < 0.0 || total < best) {
        best = total;
      }
    }
    total_ns_ = best < 0.0 ? 0.0 : best;
  }

  template <typename F> void measure_indexed (F &&fn) {
    warmup (
      [&] {
        for (int i = 0; i < runs_; ++i) {
          fn (i);
        }
      },
      default_warmup ());

    double best = -1.0;

    for (int r = 0; r < default_repeats (); ++r) {
      const Stopwatch sw;

      for (int i = 0; i < runs_; ++i) {
        fn (i);
      }
      const double total = sw.elapsed_ns ();

      if (best < 0.0 || total < best) {
        best = total;
      }
    }
    total_ns_ = best < 0.0 ? 0.0 : best;
  }

  // single timed pass for benchmarks consuming external fixtures
  template <typename F> void measure_once (F &&fn) {
    const Stopwatch sw;

    for (int i = 0; i < runs_; ++i) {
      fn ();
    }
    total_ns_ = sw.elapsed_ns ();
  }

  template <typename F> void measure_indexed_once (F &&fn) {
    const Stopwatch sw;

    for (int i = 0; i < runs_; ++i) {
      fn (i);
    }
    total_ns_ = sw.elapsed_ns ();
  }

  // nanoseconds per single operation
  double elapsed_ns () const {
    return runs_ > 0 ? total_ns_ / runs_ : total_ns_;
  }

  // nanoseconds spent for the whole batch
  double total_ns () const {
    return total_ns_;
  }
};

// number of runs each benchmark executes (configurable via --runs=n)
inline int &default_runs () {
  static int runs = 1000;
  return runs;
}

// stores a benchmark result
inline void report (const char *name, int runs, double total_ns) {
  Reporter::instance ().results.push (Result { name, g_sections.last_section, runs, runs > 0 ? total_ns / runs : total_ns, total_ns });
}

// --- benchmark output formatting ---

inline void format_duration (char *out, size_t size, double ns) {
  if (ns < 1e3) {
    snprintf (out, size, "%.2f ns", ns);
  }
  else if (ns < 1e6) {
    snprintf (out, size, "%.2f us", ns / 1e3);
  }
  else if (ns < 1e9) {
    snprintf (out, size, "%.2f ms", ns / 1e6);
  }
  else {
    snprintf (out, size, "%.2f s", ns / 1e9);
  }
}

inline void format_ops (char *out, size_t size, double ops) {
  if (ops >= 1e9) {
    snprintf (out, size, "%.2f Gops/s", ops / 1e9);
  }
  else if (ops >= 1e6) {
    snprintf (out, size, "%.2f Mops/s", ops / 1e6);
  }
  else if (ops >= 1e3) {
    snprintf (out, size, "%.2f kops/s", ops / 1e3);
  }
  else {
    snprintf (out, size, "%.2f ops/s", ops);
  }
}

// terminal width for the benchmark table (mirrors the console reporter)
inline int terminal_columns () {
#if defined(YSTL_WINDOWS)
  CONSOLE_SCREEN_BUFFER_INFO info {};

  if (GetConsoleScreenBufferInfo (GetStdHandle (STD_OUTPUT_HANDLE), &info)) {
    const int width = info.srWindow.Right - info.srWindow.Left + 1;

    if (width > 0) {
      return width;
    }
  }
#else
  struct winsize size {};

  if (ioctl (STDOUT_FILENO, TIOCGWINSZ, &size) == 0 && size.ws_col > 0) {
    return size.ws_col;
  }
#endif
  const char *columns = getenv ("COLUMNS");

  if (columns && atoi (columns) > 0) {
    return atoi (columns);
  }
  return 80;
}

// left-aligned name, truncated with an ellipsis when it does not fit
inline void print_name (const char *name, int width) {
  const int len = static_cast<int> (strlen (name));

  if (len <= width) {
    printf ("%-*s", width, name);
    return;
  }
  for (int i = 0; i < width - 3; ++i) {
    putchar (name[i]);
  }
  fputs ("...", stdout);
}

// prints recorded benchmarks [first, last) as a table with fixed column
// widths so every group lines up (only the name column flexes)
inline void print_group (size_t first, size_t last) {
  auto &results = Reporter::instance ().results;

  if (first >= last) {
    return;
  }

  constexpr int kIndent = 3;
  constexpr int kGaps = 8;
  constexpr int kRunsW = 8;
  constexpr int kTimeW = 9;
  constexpr int kOpsW = 13;
  constexpr int kTotalW = 9;
  constexpr int kFixed = kGaps + kRunsW + kTimeW + kOpsW + kTotalW;

  int w_name = terminal_columns () - 1 - kIndent - kFixed;

  if (w_name < 16) {
    w_name = 16;
  }

  printf (
    "%*s%-*s  %*s  %*s  %*s  %*s\n", kIndent, "", w_name, "benchmark", kRunsW, "runs", kTimeW, "time/op", kOpsW, "ops/s", kTotalW, "total");

  for (int i = 0, width = kIndent + w_name + kFixed; i < width; ++i) {
    putchar ('-');
  }
  putchar ('\n');

  for (size_t i = first; i < last; ++i) {
    const auto &r = results[i];

    char display[256] {};
    char runs[32] {};
    char per_op[32] {};
    char ops[32] {};
    char total[32] {};

    r.display_name (display, sizeof (display));
    snprintf (runs, sizeof (runs), "%d", r.runs);

    if (r.total_ns <= 0.0) {
      // nothing measured when batch is faster than one timer tick
      snprintf (per_op, sizeof (per_op), "n/a");
      snprintf (ops, sizeof (ops), "n/a");
      snprintf (total, sizeof (total), "n/a");
    }
    else {
      format_duration (per_op, sizeof (per_op), r.ns_per_op);
      format_ops (ops, sizeof (ops), 1e9 / r.ns_per_op);
      format_duration (total, sizeof (total), r.total_ns);
    }

    printf ("%*s", kIndent, "");
    print_name (display, w_name);
    printf ("  %*s  %*s  %*s  %*s\n", kRunsW, runs, kTimeW, per_op, kOpsW, ops, kTotalW, total);
  }
}

}
} // namespace ystl::benchmark

// --- runner ---

namespace ystl::test {

// drives the registry and forwards every event to a reporter
inline int run (const char *filter, ITestReporter &reporter) {
  auto &cases = registry ().cases ();
  auto &benchmarks = ystl::benchmark::Reporter::instance ().results;

  int selected = 0;
  int name_width = 0;

  for (const auto &tc : cases) {
    if (!filter || strstr (tc.name, filter)) {
      selected++;

      const int width = static_cast<int> (strlen (tc.name));
      name_width = width > name_width ? width : name_width;
    }
  }
  reporter.on_suite_start (selected, name_width);

  SuiteResult suite;
  const ystl::Stopwatch suite_clock {};

  for (auto &tc : cases) {
    if (filter && !strstr (tc.name, filter)) {
      continue;
    }

    g_results.current_test = tc.name;
    g_results.current_failed = false;
    g_sections.reset ();

    const int asserts_before = g_results.assertions;
    const size_t bench_before = benchmarks.size ();

    reporter.on_case_start (tc);

    const ystl::Stopwatch test_clock {};
    tc.func ();
    const double test_ms = test_clock.elapsed_ms ();

    CaseResult result {};
    result.name = tc.name;
    result.ms = test_ms;
    result.assertions = g_results.assertions - asserts_before;
    result.failed = g_results.current_failed;

    reporter.on_case_end (tc, result);

    // print the benchmark table right after the test that owns it
    if (reporter.console_output () && benchmarks.size () > bench_before) {
      ystl::benchmark::print_group (bench_before, benchmarks.size ());
    }

    if (result.failed) {
      suite.failed++;
      suite.failed_names.push (tc.name);
    }
    else {
      suite.passed++;
    }
  }

  suite.total = selected;
  suite.assertions = g_results.assertions;
  suite.wall_seconds = suite_clock.elapsed ();
  suite.benchmarks = static_cast<int> (benchmarks.size ());

  double benchmarked_ns = 0.0;
  for (const auto &b : benchmarks) {
    benchmarked_ns += b.total_ns;
  }
  suite.benchmarked_ns = benchmarked_ns;
  ystl::benchmark::format_duration (suite.benchmark_time, sizeof (suite.benchmark_time), benchmarked_ns);

  // Array is move-only, so hand the suite over rather than copying it
  last_suite () = ystl::move (suite);
  reporter.on_suite_end (last_suite ());
  return last_suite ().failed ? 1 : 0;
}

// opt-in: run every selected case in its own child process. required by
// harnesses whose singletons cannot be reset between cases (yapb); ystl
// leaves this off and runs everything in-process.
inline bool &fork_per_case () {
  static bool value = false;
  return value;
}

inline void set_fork_per_case (bool value) {
  fork_per_case () = value;
}

inline const char *&self_path () {
  static const char *value = "";
  return value;
}

inline bool child_succeeded (int status) {
#if defined(YSTL_WINDOWS)
  return status == 0;
#else
  return status != -1 && WIFEXITED (status) && WEXITSTATUS (status) == 0;
#endif
}

// spawn without a shell: names carry spaces/brackets, so quote explicitly
inline int run_child (const char *const *args) {
#if defined(YSTL_WINDOWS)
  // build a command line with every argument quoted; paths/names here
  // never contain quotes, so the simple form is exact
  ystl::String command {};

  for (int i = 0; args[i]; ++i) {
    if (i > 0) {
      command += ' ';
    }
    command += '"';
    command += args[i];
    command += '"';
  }

  STARTUPINFOA startup {};
  PROCESS_INFORMATION process {};

  startup.cb = sizeof (startup);

  if (!CreateProcessA (nullptr, const_cast<char *> (command.chars ()), nullptr, nullptr, TRUE, 0, nullptr, nullptr, &startup, &process)) {
    return -1;
  }
  WaitForSingleObject (process.hProcess, INFINITE);

  DWORD code = 1;

  GetExitCodeProcess (process.hProcess, &code);
  CloseHandle (process.hProcess);
  CloseHandle (process.hThread);

  return static_cast<int> (code);
#else
  const pid_t pid = fork ();

  if (pid < 0) {
    return -1;
  }
  if (pid == 0) {
    execv (args[0], const_cast<char *const *> (args));
    _exit (127);
  }

  int status = 0;

  if (waitpid (pid, &status, 0) < 0) {
    return -1;
  }
  return status;
#endif
}

// runs [names] as `self --filter=<name>` children, forwarding the console
// options; children are brief (one line + failures) and print no summary
inline int run_forked (const ystl::Array<const char *> &names, ConsoleReporter::Color color, bool quiet) {
  const int total = static_cast<int> (names.size ());
  const char *color_name = color == ConsoleReporter::Color::Always ? "always" : color == ConsoleReporter::Color::Never ? "never" : "auto";

  // children hand their counts back through this file (one at a time)
  char cwd[1024] {};
  ystl::String result_file {};

  ystl::plat.working_directory (cwd, sizeof (cwd));
  result_file += cwd;
  result_file += kPathSeparator;
  result_file += ".ystl_test_result";

  printf ("\n  running %d %s (separate processes)\n", total, total == 1 ? "case" : "cases");
  fflush (stdout);

  int failed = 0;
  int assertions = 0;
  ystl::Array<const char *> failed_names {};

  for (int i = 0; i < total; ++i) {
    char index_arg[32] {};
    char total_arg[32] {};
    char color_arg[32] {};
    ystl::String filter_arg {};
    ystl::String result_arg {};

    snprintf (index_arg, sizeof (index_arg), "--index=%d", i + 1);
    snprintf (total_arg, sizeof (total_arg), "--total=%d", total);
    snprintf (color_arg, sizeof (color_arg), "--color=%s", color_name);

    filter_arg += "--filter=";
    filter_arg += names[i];
    result_arg += "--result=";
    result_arg += result_file.chars ();

    const char *args[12] {};
    int count = 0;

    args[count++] = self_path ();
    args[count++] = "--brief";
    args[count++] = "--no-fork";
    args[count++] = index_arg;
    args[count++] = total_arg;
    args[count++] = color_arg;
    args[count++] = result_arg.chars ();

    if (quiet) {
      args[count++] = "--quiet";
    }
    args[count++] = filter_arg.chars ();
    args[count] = nullptr;

    const int status = run_child (args);

    // child result: "total passed failed assertions wall"
    if (FILE *fp = fopen (result_file.chars (), "rb")) {
      int t = 0;
      int p = 0;
      int f = 0;
      int a = 0;
      double w = 0.0;

      if (fscanf (fp, "%d %d %d %d %lf", &t, &p, &f, &a, &w) == 5) {
        assertions += a;
      }
      fclose (fp);
    }

    if (!child_succeeded (status)) {
      failed++;
      failed_names.push (names[i]);
    }
    fflush (stdout);
  }
  remove (result_file.chars ());

  printf ("\n  summary  %d case%s | %d passed | %d failed | %d assertions\n", total, total == 1 ? "" : "s", total - failed, failed, assertions);

  if (failed > 0) {
    printf ("\n  failed\n");

    for (const auto *name : failed_names) {
      printf ("    %s\n", name);
    }
  }
  return failed ? 1 : 0;
}

// shared argv parser: both ystl and downstream harnesses call this
inline int run_main (int argc, char **argv) {
  const char *filter = nullptr;
  const char *reporter_name = "console";
  const char *output_path = "test-results.xml";
  const char *result_path = nullptr;
  ConsoleReporter::Color color = ConsoleReporter::Color::Auto;
  bool quiet = false;
  bool brief = false;
  bool no_fork = false;
  int position_index = 0;
  int position_total = 0;

  self_path () = argv[0] ? argv[0] : "";

  for (int i = 1; i < argc; ++i) {
    const char *arg = argv[i];

    if (strcmp (arg, "--list-tests") == 0) {
      for (const auto &tc : registry ().cases ()) {
        printf ("  %s\n", tc.name);
      }
      return 0;
    }
    else if (strcmp (arg, "--reporter") == 0 && i + 1 < argc) {
      reporter_name = argv[++i];
    }
    else if (strncmp (arg, "--reporter=", 11) == 0) {
      reporter_name = arg + 11;
    }
    else if (strcmp (arg, "--output") == 0 && i + 1 < argc) {
      output_path = argv[++i];
    }
    else if (strncmp (arg, "--output=", 9) == 0) {
      output_path = arg + 9;
    }
    else if (strncmp (arg, "--result=", 9) == 0) {
      result_path = arg + 9;
    }
    else if (strncmp (arg, "--color=", 8) == 0) {
      const char *mode = arg + 8;

      if (strcmp (mode, "always") == 0) {
        color = ConsoleReporter::Color::Always;
      }
      else if (strcmp (mode, "never") == 0) {
        color = ConsoleReporter::Color::Never;
      }
      else {
        color = ConsoleReporter::Color::Auto;
      }
    }
    else if (strcmp (arg, "--quiet") == 0 || strcmp (arg, "-q") == 0) {
      quiet = true;
    }
    else if (strcmp (arg, "--no-fork") == 0) {
      no_fork = true;
    }
    else if (strcmp (arg, "--brief") == 0) {
      brief = true;
    }
    else if (strncmp (arg, "--index=", 8) == 0) {
      position_index = atoi (arg + 8);
    }
    else if (strncmp (arg, "--total=", 8) == 0) {
      position_total = atoi (arg + 8);
    }
    else if (strncmp (arg, "--runs=", 7) == 0) {
      ystl::benchmark::default_runs () = atoi (arg + 7);

      if (ystl::benchmark::default_runs () <= 0) {
        ystl::benchmark::default_runs () = 1;
      }
    }
    else if (strncmp (arg, "--warmup=", 9) == 0) {
      ystl::benchmark::default_warmup () = atoi (arg + 9);

      if (ystl::benchmark::default_warmup () < 0) {
        ystl::benchmark::default_warmup () = 0;
      }
    }
    else if (strncmp (arg, "--repeat=", 9) == 0) {
      ystl::benchmark::default_repeats () = atoi (arg + 9);

      if (ystl::benchmark::default_repeats () <= 0) {
        ystl::benchmark::default_repeats () = 1;
      }
    }
    else if (strncmp (arg, "--filter=", 9) == 0) {
      filter = arg + 9;
    }
    else if (arg[0] == '-') {
      // ignore unknown options
    }
    else {
      filter = arg;
    }
  }

  // a filter matching nothing is a wiring bug, never a pass
  ystl::Array<const char *> matched {};

  for (const auto &tc : registry ().cases ()) {
    if (!filter || strstr (tc.name, filter)) {
      matched.push (tc.name);
    }
  }

  if (filter && matched.size () == 0) {
    printf ("no test matches filter '%s'\n", filter);
    return 1;
  }

  // harnesses with non-resettable singletons (yapb) transparently run one
  // case per child process, like ctest does
  if (fork_per_case () && !no_fork && strcmp (reporter_name, "console") == 0 && matched.size () > 1) {
    return run_forked (matched, color, quiet);
  }

  if (strcmp (reporter_name, "console") == 0) {
    console_reporter ().set_color (color);
    console_reporter ().set_quiet (quiet);
    console_reporter ().set_brief (brief);
    console_reporter ().set_position (position_index, position_total);

    const int code = run (filter, console_reporter ());

    // child of a fork-per-case orchestrator: hand the counts back
    result_path_storage () = result_path ? result_path : "";

    if (result_path_storage ()[0]) {
      if (FILE *fp = fopen (result_path_storage (), "wb")) {
        fprintf (fp, "%d %d %d %d %.6f\n", last_suite ().total, last_suite ().passed, last_suite ().failed, last_suite ().assertions,
          last_suite ().wall_seconds);
        fclose (fp);
      }
    }
    return code;
  }
  if (strcmp (reporter_name, "junit") == 0) {
    JUnitReporter reporter { output_path };
    return run (filter, reporter);
  }
  printf ("unknown reporter '%s' (want console|junit)\n", reporter_name);
  return 1;
}

} // namespace ystl::test

// --- assertion macros ---

#define REQUIRE(expr) do { \
   if (expr) { g_results.pass (); } \
   else { g_results.fail (#expr, __FILE__, __LINE__); } \
} while (0)

#define REQUIRE_FALSE(expr) do { \
   if (expr) { g_results.fail ("!(" #expr ")", __FILE__, __LINE__); } \
   else { g_results.pass (); } \
} while (0)

#define CHECK(expr) do { \
   if (expr) { g_results.pass (); } \
   else { g_results.fail (#expr, __FILE__, __LINE__); } \
} while (0)

#define CHECK_FALSE(expr) do { \
   if (expr) { g_results.fail ("!(" #expr ")", __FILE__, __LINE__); } \
   else { g_results.pass (); } \
} while (0)

#define REQUIRE_NOTHROW(expr) do { \
   try { expr; g_results.pass (); } \
   catch (...) { g_results.fail ("REQUIRE_NOTHROW(" #expr ")", __FILE__, __LINE__, "threw exception"); } \
} while (0)

// --- test case macro ---

#define TEST_CASE(name) \
   static void YSTL_TEST_FUNCNAME (__LINE__) (); \
   static TestRegistrar YSTL_TEST_REGNAME (__LINE__) (name, YSTL_TEST_FUNCNAME (__LINE__), __FILE__, __LINE__); \
   static void YSTL_TEST_FUNCNAME (__LINE__) ()

#define YSTL_TEST_CONCAT_(a, b) a##b
#define YSTL_TEST_FUNCNAME(line) YSTL_TEST_CONCAT_(func_, line)
#define YSTL_TEST_REGNAME(line) YSTL_TEST_CONCAT_(reg_, line)

// --- section macro ---

// section(name) { ... }  - user's brace is the if-block body
#define SECTION(name) \
   if (g_sections.should_run (name))

// --- benchmark macros ---

#define BENCHMARK(name) \
   { const char *_bn = name; const int _br = ystl::benchmark::default_runs (); \
     auto _body = [&] () {

#define BENCHMARK_END \
     }; \
     for (int _bw = 0; _bw < ystl::benchmark::default_warmup (); ++_bw) { _body (); } \
     double _best = -1.0; \
     for (int _bq = 0; _bq < ystl::benchmark::default_repeats (); ++_bq) { \
        const ystl::Stopwatch _bsw; \
        for (int _bi = 0; _bi < _br; ++_bi) { _body (); } \
        const double _bt = _bsw.elapsed_ns (); \
        if (_best < 0.0 || _bt < _best) { _best = _bt; } \
     } \
     ystl::benchmark::report (_bn, _br, _best < 0.0 ? 0.0 : _best); \
   }

#define BENCHMARK_ADVANCED(name) \
   { const char *_bn = name; \
     ystl::benchmark::Chronometer meter (ystl::benchmark::default_runs ());

#define BENCHMARK_ADVANCED_END \
     ystl::benchmark::report (_bn, meter.runs (), meter.total_ns ()); \
   }
