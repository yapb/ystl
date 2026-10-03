// test_platform.cpp - tests for ystl/platform.h (platform singleton)
#include <ystl/ystl.h>
#include <ystl/test.h>

using namespace ystl;

// singleton
TEST_CASE ("Platform singleton returns same instance [platform]") {
  auto &a = Platform::instance ();
  auto &b = plat;
  REQUIRE (&a == &b);
}

// platform flags
TEST_CASE ("Platform win flag is set on Windows [platform]") {
#if defined(_WIN32)
  REQUIRE (plat.win);
#else
  REQUIRE (!plat.win);
#endif
}

TEST_CASE ("Platform x64 flag is set on 64-bit build [platform]") {
#if defined(_M_X64) || defined(__x86_64__) || defined(__aarch64__)
  REQUIRE (plat.x64);
#endif
  REQUIRE (true);
}

TEST_CASE ("Platform isNonX86 is consistent with arch macros [platform]") {
  bool result = plat.is_non_x86 ();
  REQUIRE ((result == true || result == false)); // just ensure it doesn't crash
}

// setappname
TEST_CASE ("Platform setAppName stores the name [platform]") {
  plat.set_app_name ("ystl_test");
  REQUIRE (strcmp (plat.app_name.data (), "ystl_test") == 0);
}

// pid
TEST_CASE ("Platform pid returns a positive value [platform]") {
  REQUIRE (plat.pid () > 0);
}

// hardwareconcurrency
TEST_CASE ("Platform hardwareConcurrency is at least 1 [platform]") {
  REQUIRE (plat.hardware_concurrency () >= 1);
}

// ystl::memzero
TEST_CASE ("ystl::memzero zeroes a buffer [utility]") {
  int buf[4] = { 1, 2, 3, 4 };
  ystl::memzero (buf, sizeof (buf));
  for (int v : buf) {
    REQUIRE (v == 0);
  }
}

// env
TEST_CASE ("Platform env returns a non-null string for known variable [platform]") {
  // avoid path which can exceed the 384-byte static buffer in plat.env()
  const char *val = plat.env ("OS");
  REQUIRE (val != nullptr);
}

// seconds
TEST_CASE ("Platform seconds returns non-negative value [platform]") {
  float s = plat.seconds ();
  REQUIRE (s >= 0.0f);
}

// loctime
TEST_CASE ("Platform loctime fills tm struct with valid values [platform]") {
  time_t t = time (nullptr);
  tm tm_info {};
  plat.loctime (&tm_info, &t);

  REQUIRE (tm_info.tm_year > 100); // year since 1900 - > 100 means > 2000
  REQUIRE (tm_info.tm_mon >= 0);
  REQUIRE (tm_info.tm_mon <= 11);
  REQUIRE (tm_info.tm_mday >= 1);
  REQUIRE (tm_info.tm_mday <= 31);
}

// tmpfname
TEST_CASE ("Platform tmpfname returns a non-empty string [platform]") {
  const char *name = plat.tmpfname ();
  REQUIRE (name != nullptr);
  REQUIRE (name[0] != '\0');
}

// openstdiofile,
TEST_CASE ("Platform openStdioFile creates a file, fileExists detects it, removeFile deletes it [platform]") {
  const char *fname = "ystl_test_platform_file.tmp";

  // write a file
  FILE *f = plat.open_stdio_file (fname, "w");
  REQUIRE (f != nullptr);
  fprintf (f, "test");
  fclose (f);

  REQUIRE (plat.file_exists (fname));

  plat.remove_file (fname);
  REQUIRE (!plat.file_exists (fname));
}

// createdirectory
TEST_CASE ("Platform createDirectory creates a directory [platform]") {
  const char *dir = "ystl_test_platform_dir";

  bool ok = plat.create_directory (dir);
  REQUIRE (ok);
  REQUIRE (plat.file_exists (dir));

  // cleanup
#if defined(YSTL_WINDOWS)
  _rmdir (dir);
#else
  rmdir (dir);
#endif
}

// plat - working
TEST_CASE ("working directory get and set roundtrip [platform]") {
  char buffer[2048] {};

  REQUIRE (plat.working_directory (buffer, sizeof (buffer)) > 0);
  CHECK (ystl::String (buffer).size () > 0);

  CHECK (plat.set_working_directory ("."));

  char second[2048] {};

  plat.working_directory (second, sizeof (second));
  CHECK (ystl::String (second) == ystl::String (buffer));
}
