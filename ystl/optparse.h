// SPDX-License-Identifier: Unlicense

#pragma once

#include <stdio.h>

#include <ystl/string.h>
#include <ystl/array.h>
#include <ystl/hashmap.h>
#include <ystl/platform.h>

namespace ystl {

// minimalistic command line option parser with python optparse-like semantics
class OptionParser final : public NonCopyable {
public:
  struct Option final {
    String short_name {};
    String long_name {};
    String dest {};
    String help {};
    String metavar {};
    bool store_true = false;
    bool store_false = false;
  };

private:
  Array<Option> options_;
  HashMap<String, String> values_;
  Array<String> args_;

  String prog_ = "program";
  String version_ = "";
  String usage_ = "usage: %prog [OPTION]... [OBJECT]...";
  String description_ = "";
  String epilog_ = "";

  bool help_requested_ = false;
  bool version_requested_ = false;
  String error_ = "";

private:
  const Option *find_option (StringRef arg) const {
    for (const auto &opt : options_) {
      if (opt.short_name.empty () && opt.long_name.empty ()) {
        continue;
      }
      if ((!opt.short_name.empty () && String (opt.short_name) == String (arg)) ||
          (!opt.long_name.empty () && String (opt.long_name) == String (arg))) {
        return &opt;
      }
    }
    return nullptr;
  }

  static String basename (StringRef path) {
    auto sep = path.find_last_of (kPathSeparator);

    if (sep != StringRef::InvalidIndex) {
      path = path.substr (sep + 1);
    }
    return String (path);
  }

public:
  OptionParser &version (StringRef value) {
    version_ = value;
    return *this;
  }

  OptionParser &usage (StringRef value) {
    usage_ = value;
    return *this;
  }

  OptionParser &description (StringRef value) {
    description_ = value;
    return *this;
  }

  OptionParser &epilog (StringRef value) {
    epilog_ = value;
    return *this;
  }

  OptionParser &add_option (StringRef long_name) {
    return add_option ("", long_name);
  }

  OptionParser &add_option (StringRef short_name, StringRef long_name) {
    options_.push (Option {});

    auto &opt = options_[options_.size () - 1];

    opt.short_name = short_name;
    opt.long_name = long_name;
    opt.dest = long_name;
    opt.dest.ltrim ("-");

    for (auto &ch : opt.dest) {
      if (ch == '-') {
        ch = '_';
      }
    }
    return *this;
  }

  Option &last_option () {
    if (options_.empty ()) [[unlikely]] {
      plat.abort ("OptionParser::last_option() called with no options");
    }
    return options_[options_.size () - 1];
  }

  OptionParser &add_version_option () {
    add_option ("-v", "--version");
    last_option ().help = "show program's version number and exit";
    return *this;
  }

  OptionParser &set_defaults (StringRef dest, StringRef value) {
    values_[String (dest)] = value;
    return *this;
  }

  // fluent helpers for the last added option
  OptionParser &dest (StringRef value) {
    last_option ().dest = value;
    return *this;
  }

  OptionParser &help (StringRef value) {
    last_option ().help = value;
    return *this;
  }

  OptionParser &metavar (StringRef value) {
    last_option ().metavar = value;
    return *this;
  }

  OptionParser &action (StringRef value) {
    last_option ().store_true = (value == "store_true");
    last_option ().store_false = (value == "store_false");
    return *this;
  }

  bool parse_args (int argc, const char *const argv[]) {
    help_requested_ = false;
    version_requested_ = false;
    error_ = "";
    args_.clear ();

    if (argc > 0 && argv[0]) {
      prog_ = basename (argv[0]);
    }

    for (int index = 1; index < argc; ++index) {
      const auto arg = String (argv[index]);

      if (arg.empty () || arg[0] != '-') {
        args_.push (arg);
        continue;
      }
      if (arg == "-h" || arg == "--help") {
        help_requested_ = true;
        return false;
      }
      if (arg == "-v" || arg == "--version") {
        version_requested_ = true;
        return false;
      }
      auto name = arg;
      auto value = String ();
      auto has_value = false;

      // split the --option=value form
      if (arg.starts_with ("--")) {
        auto eq = arg.find ('=');

        if (eq != String::InvalidIndex) {
          name = arg.substr (0, eq);
          value = arg.substr (eq + 1);
          has_value = true;
        }
      }
      const auto *opt = find_option (name);

      if (!opt) {
        error_ = strings.format ("no such option: %s", name.chars ());
        return false;
      }
      if (opt->store_true) {
        values_[opt->dest] = "1";
        continue;
      }
      if (opt->store_false) {
        values_[opt->dest] = "0";
        continue;
      }
      if (!has_value) {
        if (index + 1 >= argc) {
          error_ = strings.format ("option %s requires an argument", name.chars ());
          return false;
        }
        value = argv[++index];
      }
      values_[opt->dest] = value;
    }
    return true;
  }

  bool help_requested () const {
    return help_requested_;
  }

  bool version_requested () const {
    return version_requested_;
  }

  StringRef error () const {
    return error_;
  }

  const Array<String> &args () const {
    return args_;
  }

  bool is_set (StringRef key) const {
    return values_.contains (String (key));
  }

  bool get (StringRef key) const {
    const auto *value = values_.find (String (key));

    return value && *value == "1";
  }

  const String &operator[] (StringRef key) const {
    static const String empty = "";

    const auto *value = values_.find (String (key));

    return value ? *value : empty;
  }

  void print_version () const {
    if (!version_.empty ()) {
      printf ("%s\n", version_.chars ());
    }
  }

  void print_help () const {
    auto usage = usage_;

    if (usage.find ("%prog") != String::InvalidIndex) {
      usage.replace ("%prog", prog_);
    }
    printf ("%s\n", usage.chars ());

    if (!description_.empty ()) {
      printf ("\n%s\n", description_.chars ());
    }
    printf ("\n");

    if (options_.empty ()) {
      return;
    }
    printf ("optional arguments:\n");

    for (const auto &opt : options_) {
      auto label = String ();

      if (!opt.short_name.empty ()) {
        label += opt.short_name;
      }
      if (!opt.long_name.empty ()) {
        if (!label.empty ()) {
          label += ", ";
        }
        label += opt.long_name;
      }
      if (!opt.metavar.empty ()) {
        label += strings.format (" %s", opt.metavar.chars ());
      }
      printf ("  %s", label.chars ());

      if (label.size () > 22) {
        printf ("\n");
      }
      else {
        printf ("%*s", static_cast<int> (23 - label.size ()), "");
      }

      if (opt.help.empty ()) {
        printf ("\n");
      }
      else {
        printf ("%s\n", opt.help.chars ());
      }
    }
    if (!epilog_.empty ()) {
      printf ("\n%s\n", epilog_.chars ());
    }
  }
};

}
