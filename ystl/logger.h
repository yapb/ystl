// SPDX-License-Identifier: Unlicense

#pragma once

#include <time.h>

#include <ystl/files.h>
#include <ystl/lambda.h>
#include <ystl/singleton.h>

namespace ystl {

class SimpleLogger final : public Singleton<SimpleLogger> {
public:
  using PrintFunction = Lambda<void (const char *)>;

private:
  String filename_;
  PrintFunction print_fun_;
  bool log_write_enabled_ = true;

public:
  explicit SimpleLogger () = default;
  ~SimpleLogger () = default;

public:
  class LogFile final {
  private:
    File handle_;

  public:
    LogFile (StringRef filename) {
      handle_.open (filename, "at");
    }

    ~LogFile () {
      handle_.close ();
    }

  public:
    void print (StringRef msg) {
      if (!handle_) {
        return;
      }
      handle_.write (msg.chars (), msg.size ());
    }
  };

private:
  void log_to_file (const char *level, const char *msg) {
    if (!log_write_enabled_) {
      return;
    }

    time_t ticks = time (&ticks);
    tm timeinfo {};

    plat.loctime (&timeinfo, &ticks);

    char timebuf[32] {};
    strftime (timebuf, sizeof timebuf, "%Y-%m-%d %H:%M:%S", &timeinfo);

    LogFile lf (filename_);

    // dynamic buffer, the decorated line can exceed any static limit
    String line {};
    line.assignf ("%s (%s): %s\n", timebuf, level, msg);
    lf.print (line);
  }

public:
  enum class LogLevel {
    Fatal,
    Error,
    Info
  };

  template <LogLevel level, typename... Args> void log (const char *fmt, Args &&...args) {
    // dynamic buffer, log lines echo unbounded user content
    String msg {};
    msg.assignf (fmt, ystl::forward<Args> (args)...);
    constexpr const char *level_str = level == LogLevel::Fatal ? "FATAL" : level == LogLevel::Error ? "ERROR" : "INFO";

    log_to_file (level_str, msg.chars ());

    if constexpr (level == LogLevel::Fatal) {
      plat.abort (msg.chars ());
    }

    if constexpr (level != LogLevel::Fatal) {
      if (print_fun_) {
        print_fun_ (msg.chars ());
      }
    }
  }

  template <typename... Args> void fatal (const char *fmt, Args &&...args) {
    log<LogLevel::Fatal> (fmt, ystl::forward<Args> (args)...);
  }

  template <typename... Args> void error (const char *fmt, Args &&...args) {
    log<LogLevel::Error> (fmt, ystl::forward<Args> (args)...);
  }

  template <typename... Args> void message (const char *fmt, Args &&...args) {
    log<LogLevel::Info> (fmt, ystl::forward<Args> (args)...);
  }

public:
  void initialize (StringRef filename, PrintFunction print_function) {
    print_fun_ = ystl::move (print_function);
    filename_ = filename;
    log_write_enabled_ = true;
  }

  void set_log_write_enabled (bool enabled) {
    log_write_enabled_ = enabled;
  }
};

// expose global instance
YSTL_EXPOSE_GLOBAL_SINGLETON (SimpleLogger, logger);

}
