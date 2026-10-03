// test_optparse.cpp - tests for ystl/optparse.h command line option parser
#include <ystl/ystl.h>
#include <ystl/test.h>

using ystl::OptionParser;

// the parser needs stable argv, we keep the backing storage around between calls
static ystl::Array<ystl::String> argv_storage;
static ystl::Array<const char *> argv_pointers;
static int argc_ = 0;

static int setup_args (std::initializer_list<const char *> items) {
  argv_storage.clear ();
  argv_storage.push ("gtool");

  for (auto *item : items) {
    argv_storage.push (item);
  }
  argv_pointers.clear ();

  for (auto &str : argv_storage) {
    argv_pointers.push (str.chars ());
  }
  argc_ = static_cast<int> (argv_pointers.size ());

  return argc_;
}

static const char **argv_pointer () {
  return argv_pointers.data ();
}

TEST_CASE ("parses store options [optparse]") {
  OptionParser parser;

  parser.add_option ("-C", "--convert");

  setup_args ({ "-C", "de_dust.pwf" });
  REQUIRE (parser.parse_args (argc_, argv_pointer ()));
  CHECK (parser["convert"] == "de_dust.pwf");
  CHECK (parser.is_set ("convert"));
  CHECK (!parser.get ("convert"));
}

TEST_CASE ("parses long options with equals sign [optparse]") {
  OptionParser parser;

  parser.add_option ("", "--convert");

  setup_args ({ "--convert=de_dust.pwf" });
  REQUIRE (parser.parse_args (argc_, argv_pointer ()));
  CHECK (parser["convert"] == "de_dust.pwf");
}

TEST_CASE ("store true and store false actions [optparse]") {
  OptionParser parser;

  parser.set_defaults ("print_summ", "1");
  parser.add_option ("--no-print-summary").action ("store_false").dest ("print_summ");
  parser.add_option ("--enable-log").action ("store_true").dest ("enable_log");

  setup_args ({ "--no-print-summary", "--enable-log" });
  REQUIRE (parser.parse_args (argc_, argv_pointer ()));
  CHECK (parser.get ("print_summ") == false);
  CHECK (parser.get ("enable_log") == true);
}

TEST_CASE ("defaults are respected when options are not passed [optparse]") {
  OptionParser parser;

  parser.set_defaults ("fix_nodes", "1");
  parser.set_defaults ("save_conf", "0");
  parser.add_option ("--no-fix-nodes").action ("store_false").dest ("fix_nodes");
  parser.add_option ("--save-conf").action ("store_true").dest ("save_conf");

  setup_args ({});
  REQUIRE (parser.parse_args (argc_, argv_pointer ()));
  CHECK (parser.get ("fix_nodes"));
  CHECK (!parser.get ("save_conf"));
  CHECK (!parser.is_set ("missing_key"));
}

TEST_CASE ("positional arguments are collected [optparse]") {
  OptionParser parser;

  parser.add_option ("-V", "--verify");

  setup_args ({ "free", "positional", "-V", "file" });
  REQUIRE (parser.parse_args (argc_, argv_pointer ()));
  REQUIRE (parser.args ().size () == 2);
  CHECK (parser.args ()[0] == "free");
  CHECK (parser.args ()[1] == "positional");
  CHECK (parser["verify"] == "file");
}

TEST_CASE ("unknown options are reported [optparse]") {
  OptionParser parser;

  parser.add_option ("-C", "--convert");

  setup_args ({ "--no-such-option" });
  CHECK (!parser.parse_args (argc_, argv_pointer ()));
  CHECK (ystl::String (parser.error ()).find ("no such option") != ystl::String::InvalidIndex);
}

TEST_CASE ("missing option value is reported [optparse]") {
  OptionParser parser;

  parser.add_option ("-C", "--convert");

  setup_args ({ "-C" });
  CHECK (!parser.parse_args (argc_, argv_pointer ()));
  CHECK (ystl::String (parser.error ()).find ("requires an argument") != ystl::String::InvalidIndex);
}

TEST_CASE ("dest is derived from the long name [optparse]") {
  OptionParser parser;

  parser.add_option ("", "--generate-credits");

  CHECK (parser.last_option ().dest == "generate_credits");
}

TEST_CASE ("help and version requests [optparse]") {
  OptionParser parser;

  parser.version ("tool 1.0");
  parser.add_version_option ();

  setup_args ({ "--version" });
  CHECK (!parser.parse_args (argc_, argv_pointer ()));
  CHECK (parser.version_requested ());

  setup_args ({ "-h" });
  CHECK (!parser.parse_args (argc_, argv_pointer ()));
  CHECK (parser.help_requested ());
}

TEST_CASE ("last value wins for repeated options [optparse]") {
  OptionParser parser;

  parser.add_option ("-C", "--convert");

  setup_args ({ "-C", "first", "--convert=second" });
  REQUIRE (parser.parse_args (argc_, argv_pointer ()));
  CHECK (parser["convert"] == "second");
}

TEST_CASE ("empty --opt= keeps empty value instead of eating next arg [optparse]") {
  OptionParser parser;

  parser.add_option ("-o", "--output");

  setup_args ({ "--output=" });
  REQUIRE (parser.parse_args (argc_, argv_pointer ()));
  CHECK (parser["output"] == "");
}
