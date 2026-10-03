// test_logger.cpp - tests for ystl/logger.h (simplelogger)
#include <ystl/ystl.h>
#include <ystl/test.h>

using namespace ystl;

// helper: get a unique
static const char *log_file_name () {
  return "ystl_test_logger.tmp";
}

static void cleanup_log () {
  plat.remove_file (log_file_name ());
}

// singleton
TEST_CASE ("SimpleLogger singleton returns same instance [logger]") {
  auto &a = SimpleLogger::instance ();
  auto &b = logger;
  REQUIRE (&a == &b);
}

// initialize +
TEST_CASE ("SimpleLogger initialize does not crash [logger]") {
  logger.initialize (log_file_name (), nullptr);
  REQUIRE (true);
  cleanup_log ();
}

TEST_CASE ("SimpleLogger set_log_write_enabled disables file output [logger]") {
  cleanup_log ();
  logger.initialize (log_file_name (), nullptr);
  logger.set_log_write_enabled (false);

  logger.message ("This should not appear in the file");

  // file should not have been created because writing is disabled
  REQUIRE (!plat.file_exists (log_file_name ()));

  logger.set_log_write_enabled (true);
}

// message
TEST_CASE ("SimpleLogger message writes to log file [logger]") {
  cleanup_log ();
  logger.initialize (log_file_name (), nullptr);
  logger.set_log_write_enabled (true);

  logger.message ("test message %d", 42);

  // file should now exist
  REQUIRE (plat.file_exists (log_file_name ()));
  cleanup_log ();
}

// error
TEST_CASE ("SimpleLogger error writes to log file [logger]") {
  cleanup_log ();
  logger.initialize (log_file_name (), nullptr);
  logger.set_log_write_enabled (true);

  logger.error ("test error %s", "hello");

  REQUIRE (plat.file_exists (log_file_name ()));
  cleanup_log ();
}

// print function callback
TEST_CASE ("SimpleLogger invokes the print function on message [logger]") {
  cleanup_log ();

  bool called = false;
  String captured;

  logger.initialize (log_file_name (), [&called, &captured] (const char *msg) {
    called = true;
    captured = msg;
  });
  logger.set_log_write_enabled (true);

  logger.message ("callback_test");

  REQUIRE (called);
  REQUIRE (captured.contains ("callback_test"));
  cleanup_log ();
}

TEST_CASE ("SimpleLogger invokes the print function on error [logger]") {
  cleanup_log ();

  bool called = false;
  logger.initialize (log_file_name (), [&called] (const char *) {
    called = true;
  });
  logger.set_log_write_enabled (true);

  logger.error ("some error");
  REQUIRE (called);
  cleanup_log ();
}

// message with nullptr
TEST_CASE ("SimpleLogger message with nullptr print function does not crash [logger]") {
  cleanup_log ();
  logger.initialize (log_file_name (), nullptr);
  logger.set_log_write_enabled (true);
  logger.message ("no callback");
  REQUIRE (true);
  cleanup_log ();
}

// logfile - open and
TEST_CASE ("SimpleLogger LogFile prints to a file directly [logger]") {
  const char *fname = "ystl_test_logfile.tmp";
  {
    SimpleLogger::LogFile lf (fname);
    lf.print ("direct write");
  }
  REQUIRE (plat.file_exists (fname));

  // verify content
  String content = FileLoader::load_to_string (fname);
  REQUIRE (content.contains ("direct write"));

  plat.remove_file (fname);
}
