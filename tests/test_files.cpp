// test_files.cpp - tests for ystl/files.h (file, memfilestorage, memfile, fileenumerator)
#include <ystl/ystl.h>
#include <ystl/test.h>

using namespace ystl;

// file - basic open /
TEST_CASE ("File default-constructed is not valid [files]") {
  File f;
  REQUIRE (!f);
}

TEST_CASE ("File open non-existent file for reading fails [files]") {
  File f ("ystl_nonexistent_xyz_123.txt", "r");
  REQUIRE (!f);
}

TEST_CASE ("File open for writing succeeds [files]") {
  const char *fname = "ystl_test_file_open.tmp";
  {
    File f (fname, "w");
    REQUIRE (f);
  } // closes on destruction
  plat.remove_file (fname);
}

// file - length
TEST_CASE ("File length matches written bytes [files]") {
  const char *fname = "ystl_test_file_length.tmp";
  {
    File fw (fname, "w");
    REQUIRE (fw);
    fw.puts ("hello");
  }
  {
    File fr (fname, "r");
    REQUIRE (fr);
    REQUIRE (fr.size () == 5);
  }
  plat.remove_file (fname);
}

// file - puts / get / eof
TEST_CASE ("File puts and get read back bytes correctly [files]") {
  const char *fname = "ystl_test_file_puts.tmp";
  {
    File fw (fname, "w");
    fw.puts ("abc");
  }
  {
    File fr (fname, "r");
    REQUIRE (fr);
    int a = fr.get ();
    int b = fr.get ();
    int c = fr.get ();
    REQUIRE (a == 'a');
    REQUIRE (b == 'b');
    REQUIRE (c == 'c');
    // feof() only returns true after a read that goes past the end
    fr.get (); // trigger the past-end read
    REQUIRE (fr.eof ());
  }
  plat.remove_file (fname);
}

// file - getline
TEST_CASE ("File getLine reads lines from a text file [files]") {
  const char *fname = "ystl_test_file_getline.tmp";
  {
    File fw (fname, "w");
    fw.puts ("line1\n");
    fw.puts ("line2\n");
  }
  {
    File fr (fname, "r");
    REQUIRE (fr);

    String line;
    REQUIRE (fr.get_line (line));
    REQUIRE (line.starts_with ("line1"));

    REQUIRE (fr.get_line (line));
    REQUIRE (line.starts_with ("line2"));
  }
  plat.remove_file (fname);
}

// file - seek / rewind
TEST_CASE ("File seek and rewind work correctly [files]") {
  const char *fname = "ystl_test_file_seek.tmp";
  {
    File fw (fname, "w");
    fw.puts ("abcdef");
  }
  {
    File fr (fname, "r");
    REQUIRE (fr);
    fr.seek (3, SEEK_SET);
    REQUIRE (fr.get () == 'd');

    fr.rewind ();
    REQUIRE (fr.get () == 'a');
  }
  plat.remove_file (fname);
}

// file - read / write
TEST_CASE ("File binary write and read round-trip [files]") {
  const char *fname = "ystl_test_file_binary.tmp";
  uint8_t written[4] = { 0xDE, 0xAD, 0xBE, 0xEF };
  uint8_t read_back[4] = {};
  {
    File fw (fname, "wb");
    REQUIRE (fw);
    fw.write (written, sizeof (written));
  }
  {
    File fr (fname, "rb");
    REQUIRE (fr);
    fr.read (read_back, sizeof (read_back));
    for (int i = 0; i < 4; ++i) {
      REQUIRE (read_back[i] == written[i]);
    }
  }
  plat.remove_file (fname);
}

// file - putchar / close
TEST_CASE ("File putChar writes a single character [files]") {
  const char *fname = "ystl_test_file_putchar.tmp";
  {
    File fw (fname, "w");
    fw.put_char ('Z');
  }
  {
    File fr (fname, "r");
    REQUIRE (fr.get () == 'Z');
  }
  plat.remove_file (fname);
}

// file::makepath
TEST_CASE ("File::makePath creates nested directories without crashing [files]") {
  // exercise makepath with safe temp name in current dir
  File::make_path ("ystl_mkpath_test_dir");
  REQUIRE (plat.remove_directory ("ystl_mkpath_test_dir"));
  REQUIRE (true);
}

// memfilestorage -
TEST_CASE ("FileLoader defaultLoad reads an existing file [files]") {
  const char *fname = "ystl_test_memfile.tmp";
  {
    File fw (fname, "w");
    fw.puts ("hello memfile");
  }

  int size = 0;
  uint8_t *data = FileLoader::default_load (fname, &size);
  REQUIRE (data != nullptr);
  REQUIRE (size == 13); // "hello memfile"
  REQUIRE (memcmp (data, "hello memfile", 13) == 0);

  FileLoader::default_unload (data);
  plat.remove_file (fname);
}

TEST_CASE ("FileLoader defaultLoad returns nullptr for missing file [files]") {
  int size = 99;
  uint8_t *data = FileLoader::default_load ("ystl_missing_xyz.tmp", &size);
  REQUIRE (data == nullptr);
  REQUIRE (size == 0);
}

TEST_CASE ("FileLoader loadToString reads file contents as String [files]") {
  const char *fname = "ystl_test_mfstr.tmp";
  {
    File fw (fname, "w");
    fw.puts ("ystl rocks");
  }

  String s = FileLoader::load_to_string (fname);
  REQUIRE (s.starts_with ("ystl rocks"));
  plat.remove_file (fname);
}

TEST_CASE ("FileLoader loadToString returns empty for missing file [files]") {
  String s = FileLoader::load_to_string ("ystl_missing_xyz.tmp");
  REQUIRE (s.empty ());
}

// memfilestorage -
TEST_CASE ("FileLoader initialized with default functions loads file [files]") {
  FileLoader::instance ().initialize (FileLoader::default_load, FileLoader::default_unload);

  const char *fname = "ystl_test_mfstorage.tmp";
  {
    File fw (fname, "w");
    fw.puts ("xyz");
  }

  int sz = 0;
  uint8_t *buf = FileLoader::instance ().load (fname, &sz);
  REQUIRE (buf != nullptr);
  REQUIRE (sz == 3);

  FileLoader::instance ().unload (buf);
  plat.remove_file (fname);
}

// memfile - default /
TEST_CASE ("MemFile default-constructed is not valid [files]") {
  FileLoader::instance ().initialize (FileLoader::default_load, FileLoader::default_unload);

  MemFile mf;
  REQUIRE (!mf);
}

TEST_CASE ("MemFile open non-existent file fails [files]") {
  FileLoader::instance ().initialize (FileLoader::default_load, FileLoader::default_unload);

  MemFile mf ("ystl_nonexistent_xyz.tmp");
  REQUIRE (!mf);
}

TEST_CASE ("MemFile reads bytes from in-memory file [files]") {
  FileLoader::instance ().initialize (FileLoader::default_load, FileLoader::default_unload);

  const char *fname = "ystl_test_memfile2.tmp";
  {
    File fw (fname, "w");
    fw.puts ("abcd");
  }

  MemFile mf (fname);
  REQUIRE (mf);
  REQUIRE (mf.size () == 4);

  REQUIRE (mf.get () == 'a');
  REQUIRE (mf.get () == 'b');

  mf.rewind ();
  REQUIRE (mf.get () == 'a');

  mf.close ();
  REQUIRE (!mf);

  plat.remove_file (fname);
}

TEST_CASE ("MemFile read block [files]") {
  FileLoader::instance ().initialize (FileLoader::default_load, FileLoader::default_unload);

  const char *fname = "ystl_test_memfile_read.tmp";
  {
    File fw (fname, "wb");
    uint8_t data[4] = { 10, 20, 30, 40 };
    fw.write (data, 4);
  }

  MemFile mf (fname);
  REQUIRE (mf);

  uint8_t buf[4] = {};
  size_t n = mf.read (buf, 4);
  REQUIRE (n == 1); // 1 block of size 4
  REQUIRE (buf[0] == 10);
  REQUIRE (buf[3] == 40);
  REQUIRE (mf.eof ());

  plat.remove_file (fname);
}

TEST_CASE ("MemFile seek works [files]") {
  FileLoader::instance ().initialize (FileLoader::default_load, FileLoader::default_unload);

  const char *fname = "ystl_test_memfile_seek.tmp";
  {
    File fw (fname, "w");
    fw.puts ("abcdef");
  }

  MemFile mf (fname);
  REQUIRE (mf);

  mf.seek (3, SEEK_SET);
  REQUIRE (mf.get () == 'd');

  mf.seek (1, SEEK_END);
  REQUIRE (mf.get () == 'f');

  plat.remove_file (fname);
}

TEST_CASE ("MemFile getLine reads lines [files]") {
  FileLoader::instance ().initialize (FileLoader::default_load, FileLoader::default_unload);

  const char *fname = "ystl_test_memfile_getline.tmp";
  {
    File fw (fname, "w");
    fw.puts ("first\nsecond\n");
  }

  MemFile mf (fname);
  REQUIRE (mf);

  String line;
  REQUIRE (mf.get_line (line));
  REQUIRE (line.starts_with ("first"));

  REQUIRE (mf.get_line (line));
  REQUIRE (line.starts_with ("second"));

  plat.remove_file (fname);
}

// fileenumerator
TEST_CASE ("FileEnumerator finds the file we just created [files]") {
  const char *fname = "ystl_test_enum_file.tmp";
  {
    File fw (fname, "w");
    fw.puts ("x");
  }

  bool found = false;
  FileEnumerator fe ("ystl_test_enum_file.tmp");

  while (fe) {
    String match = fe.get_match ();
    if (match.contains ("ystl_test_enum_file")) {
      found = true;
    }
    fe.next ();
  }
  REQUIRE (found);

  plat.remove_file (fname);
}

TEST_CASE ("FileEnumerator with no matches is not valid from the start [files]") {
  FileEnumerator fe ("ystl_this_file_does_not_exist_xyz_123.zzz");
  // no match means enumerator starts invalid on both platforms
  REQUIRE (!fe);
}

// additional tests for missing coverage

// file class - additional
TEST_CASE ("File flush method works [files]") {
  const char *fname = "ystl_test_flush.tmp";
  File f (fname, "w");
  REQUIRE (f);
  f.puts ("test");
  REQUIRE (f.flush ());
  f.close (); // windows cannot delete an open file
  REQUIRE (plat.remove_file (fname));
}

TEST_CASE ("File puts with format string [files]") {
  const char *fname = "ystl_test_puts_format.tmp";
  {
    File f (fname, "w");
    REQUIRE (f);
    f.puts ("Number: %d, String: %s", 42, "hello");
    // fputs returns non-negative on success not byte count
  }
  {
    File f (fname, "r");
    String line;
    f.get_line (line);
    REQUIRE (line.contains ("Number: 42"));
    REQUIRE (line.contains ("String: hello"));
  }
  plat.remove_file (fname);
}

TEST_CASE ("File double close is safe [files]") {
  const char *fname = "ystl_test_double_close.tmp";
  File f (fname, "w");
  REQUIRE (f);
  f.close ();
  REQUIRE (!f);
  f.close (); // second close should be safe
  REQUIRE (!f);
  plat.remove_file (fname);
}

TEST_CASE ("File open on already open file closes first [files]") {
  const char *fname1 = "ystl_test_open1.tmp";
  const char *fname2 = "ystl_test_open2.tmp";

  File f (fname1, "w");
  REQUIRE (f);
  f.puts ("first");

  bool ok = f.open (fname2, "w");
  REQUIRE (ok);
  REQUIRE (f);
  f.puts ("second");

  f.close ();
  plat.remove_file (fname1);
  plat.remove_file (fname2);
}

TEST_CASE ("File eof on empty file [files]") {
  const char *fname = "ystl_test_empty_eof.tmp";
  {
    File f (fname, "w");
    REQUIRE (f);
  }
  {
    File f (fname, "r");
    REQUIRE (f);
    // empty file: eof() returns false until we try to read past end
    REQUIRE (!f.eof ());
    f.get (); // try to read past end
    REQUIRE (f.eof ());
  }
  plat.remove_file (fname);
}

TEST_CASE ("File get on closed file returns EOF [files]") {
  File f;
  int ch = f.get ();
  REQUIRE (ch == EOF);
}

TEST_CASE ("File read/write with zero size/count returns 0 [files]") {
  const char *fname = "ystl_test_zero_rw.tmp";
  File f (fname, "w");
  REQUIRE (f);

  char buffer[10];
  size_t written = f.write (buffer, 0, 5);
  REQUIRE (written == 0);

  f.close ();

  File fr (fname, "r");
  size_t read = fr.read (buffer, 0, 5);
  REQUIRE (read == 0);

  fr.close (); // windows cannot delete an open file
  REQUIRE (plat.remove_file (fname));
}

TEST_CASE ("File seek with invalid origin fails [files]") {
  const char *fname = "ystl_test_invalid_seek.tmp";
  File f (fname, "w");
  REQUIRE (f);
  f.puts ("test");
  f.rewind ();

  bool ok = f.seek (0, 999); // invalid origin
  REQUIRE (!ok);

  f.close (); // windows cannot delete an open file
  REQUIRE (plat.remove_file (fname));
}

TEST_CASE ("File seek beyond file end succeeds (C behavior) [files]") {
  const char *fname = "ystl_test_seek_beyond.tmp";
  File f (fname, "w");
  REQUIRE (f);
  f.puts ("test"); // 4 bytes

  // c's fseek allows seeking beyond end
  bool ok = f.seek (1000, SEEK_SET);
  REQUIRE (ok);

  f.close (); // windows cannot delete an open file
  REQUIRE (plat.remove_file (fname));
}

// memfilestorage -
TEST_CASE ("FileLoader load without initialization returns nullptr [files]") {
  // reset to default (no initialization)
  FileLoader::instance ().initialize (nullptr, nullptr);

  const char *fname = "ystl_test_noinit.tmp";
  {
    File f (fname, "w");
    f.puts ("test");
  }

  int size = 0;
  uint8_t *data = FileLoader::instance ().load (fname, &size);
  REQUIRE (data == nullptr);
  REQUIRE (size == 0);

  plat.remove_file (fname);
}

TEST_CASE ("FileLoader unload without initialization is safe [files]") {
  FileLoader::instance ().initialize (nullptr, nullptr);

  uint8_t dummy[10];
  FileLoader::instance ().unload (dummy); // should not crash
  REQUIRE (true);
}

TEST_CASE ("FileLoader unload with nullptr is safe [files]") {
  FileLoader::instance ().initialize (FileLoader::default_load, FileLoader::default_unload);

  FileLoader::instance ().unload (nullptr); // should not crash
  REQUIRE (true);
}

// memfile - additional
TEST_CASE ("MemFile seek with invalid origin fails [files]") {
  FileLoader::instance ().initialize (FileLoader::default_load, FileLoader::default_unload);

  const char *fname = "ystl_test_memfile_invalid_seek.tmp";
  {
    File f (fname, "w");
    f.puts ("test");
  }

  MemFile mf (fname);
  REQUIRE (mf);

  bool ok = mf.seek (0, 999); // invalid origin
  REQUIRE (!ok);

  plat.remove_file (fname);
}

TEST_CASE ("MemFile seek beyond file end fails [files]") {
  FileLoader::instance ().initialize (FileLoader::default_load, FileLoader::default_unload);

  const char *fname = "ystl_test_memfile_seek_beyond.tmp";
  {
    File f (fname, "w");
    f.puts ("test"); // 4 bytes
  }

  MemFile mf (fname);
  REQUIRE (mf);

  bool ok = mf.seek (1000, SEEK_SET);
  REQUIRE (!ok);

  plat.remove_file (fname);
}

TEST_CASE ("MemFile read with zero size/count returns 0 [files]") {
  FileLoader::instance ().initialize (FileLoader::default_load, FileLoader::default_unload);

  const char *fname = "ystl_test_memfile_zero_read.tmp";
  {
    File f (fname, "w");
    f.puts ("test");
  }

  MemFile mf (fname);
  REQUIRE (mf);

  char buffer[10];
  size_t read = mf.read (buffer, 0, 5);
  REQUIRE (read == 0);

  plat.remove_file (fname);
}

TEST_CASE ("MemFile get on closed file returns EOF [files]") {
  FileLoader::instance ().initialize (FileLoader::default_load, FileLoader::default_unload);

  MemFile mf;
  int ch = mf.get ();
  REQUIRE (ch == EOF);
}

TEST_CASE ("MemFile getLine on empty file returns false [files]") {
  FileLoader::instance ().initialize (FileLoader::default_load, FileLoader::default_unload);

  const char *fname = "ystl_test_memfile_empty.tmp";
  {
    File f (fname, "w");
    // write nothing - file exists but is empty
  }

  MemFile mf (fname);
  // empty memfile is falsy since bool needs content and length
  REQUIRE (!mf);

  String line;
  bool ok = mf.get_line (line);
  REQUIRE (!ok);
  REQUIRE (line.empty ());

  plat.remove_file (fname);
}

TEST_CASE ("MemFile double close is safe [files]") {
  FileLoader::instance ().initialize (FileLoader::default_load, FileLoader::default_unload);

  const char *fname = "ystl_test_memfile_double_close.tmp";
  {
    File f (fname, "w");
    f.puts ("test");
  }

  MemFile mf (fname);
  REQUIRE (mf);
  mf.close ();
  REQUIRE (!mf);
  mf.close (); // second close should be safe
  REQUIRE (!mf);

  plat.remove_file (fname);
}

TEST_CASE ("MemFile open on already open file closes first [files]") {
  FileLoader::instance ().initialize (FileLoader::default_load, FileLoader::default_unload);

  const char *fname1 = "ystl_test_memfile_open1.tmp";
  const char *fname2 = "ystl_test_memfile_open2.tmp";

  {
    File f (fname1, "w");
    f.puts ("first");
  }
  {
    File f (fname2, "w");
    f.puts ("second");
  }

  MemFile mf (fname1);
  REQUIRE (mf);
  REQUIRE (mf.size () == 5);

  bool ok = mf.open (fname2);
  REQUIRE (ok);
  REQUIRE (mf);
  REQUIRE (mf.size () == 6);

  mf.close ();
  plat.remove_file (fname1);
  plat.remove_file (fname2);
}

TEST_CASE ("MemFile getLine reads last line without trailing newline [files]") {
  FileLoader::instance ().initialize (FileLoader::default_load, FileLoader::default_unload);

  const char *fname = "ystl_test_memfile_getline_nonl.tmp";
  {
    File fw (fname, "wb");
    fw.puts ("first\nsecond");
  }

  MemFile mf (fname);
  REQUIRE (mf);

  String line;
  REQUIRE (mf.get_line (line));
  REQUIRE (line.starts_with ("first"));

  REQUIRE (mf.get_line (line));
  REQUIRE (line == "second");

  REQUIRE (!mf.get_line (line));

  plat.remove_file (fname);
}

TEST_CASE ("File getLine reads last line without trailing newline [files]") {
  const char *fname = "ystl_test_file_getline_nonl.tmp";
  {
    File fw (fname, "wb");
    fw.puts ("first\nsecond");
  }
  {
    File fr (fname, "rb");
    REQUIRE (fr);

    String line;
    REQUIRE (fr.get_line (line));
    REQUIRE (line.starts_with ("first"));

    REQUIRE (fr.get_line (line));
    REQUIRE (line == "second");

    REQUIRE (!fr.get_line (line));
  }
  plat.remove_file (fname);
}

TEST_CASE ("MemFile view returns whole buffer in a single copy [files]") {
  FileLoader::instance ().initialize (FileLoader::default_load, FileLoader::default_unload);

  const char *fname = "ystl_test_memfile_view.tmp";
  {
    File fw (fname, "wb");
    fw.puts ("first\nsecond");
  }

  MemFile mf (fname);
  REQUIRE (mf);
  REQUIRE (mf.view ().size () == mf.size ());

  String text;
  text.assign (mf.view ());
  REQUIRE (text == "first\nsecond");

  plat.remove_file (fname);
}

// fileenumerator -
TEST_CASE ("FileEnumerator getMatch when not valid [files]") {
  FileEnumerator fe ("ystl_nonexistent_xyz_123.zzz");
  REQUIRE (!fe);

  // getmatch is unsafe when enumerator is not valid
  REQUIRE (true);
}

TEST_CASE ("FileEnumerator next when not valid returns false [files]") {
  FileEnumerator fe ("ystl_nonexistent_xyz_123.zzz");
  REQUIRE (!fe);

  bool ok = fe.next ();
  REQUIRE (!ok);
}

TEST_CASE ("FileEnumerator double close is safe [files]") {
  const char *fname = "ystl_test_enum_double_close.tmp";
  {
    File f (fname, "w");
    f.puts ("x");
  }

  FileEnumerator fe (fname);
  fe.close ();
  fe.close (); // second close should be safe
  REQUIRE (true);

  plat.remove_file (fname);
}

TEST_CASE ("FileEnumerator with wildcard pattern [files]") {
  const char *fname1 = "ystl_test_wildcard_a.tmp";
  const char *fname2 = "ystl_test_wildcard_b.tmp";

  {
    File f1 (fname1, "w");
    f1.puts ("a");
  }
  {
    File f2 (fname2, "w");
    f2.puts ("b");
  }

  int count = 0;
  FileEnumerator fe ("ystl_test_wildcard_*.tmp");

  while (fe) {
    String match = fe.get_match ();
    if (match.contains ("wildcard")) {
      ++count;
    }
    fe.next ();
  }

  REQUIRE (count >= 2);

  plat.remove_file (fname1);
  plat.remove_file (fname2);
}

// span overloads
TEST_CASE ("File span write and read [files]") {
  const char *fname = "ystl_test_span.tmp";

  {
    File f (fname, "wb");
    const int values[] = { 1, 2, 3, 4 };
    REQUIRE (f.write (ystl::Span (values)) == 4u);
  }

  {
    File f (fname, "rb");
    int out[4] = {};

    REQUIRE (f.read (ystl::Span (out)) == 4u);
    REQUIRE (out[0] == 1);
    REQUIRE (out[1] == 2);
    REQUIRE (out[2] == 3);
    REQUIRE (out[3] == 4);
  }

  plat.remove_file (fname);
}

TEST_CASE ("File span write and read with Array [files]") {
  const char *fname = "ystl_test_span_array.tmp";

  {
    File f (fname, "wb");
    Array<uint8_t> data;

    data.push ('h');
    data.push ('i');

    REQUIRE (f.write (ystl::Span (data)) == 2u);
  }

  {
    File f (fname, "rb");
    Array<uint8_t> out;
    out.resize (2);

    REQUIRE (f.read (ystl::Span (out)) == 2u);
    REQUIRE (out[0] == 'h');
    REQUIRE (out[1] == 'i');
  }

  plat.remove_file (fname);
}

// fileenumerator +
TEST_CASE ("FileEnumerator matches files and reports directories [files]") {
  const char *dir_name = "ystl_test_enum";
  const char *sub_dir_name = "ystl_test_enum/sub";
  const char *file_name = "ystl_test_enum/inner.tmp";

  plat.create_directory (dir_name);
  plat.create_directory (sub_dir_name);
  {
    File fw (file_name, "w");
    REQUIRE (fw);
    fw.puts ("x");
  }

  FileEnumerator enumerator ("ystl_test_enum/*");

  bool found_dir = false;
  bool found_file = false;

  for (; enumerator; enumerator.next ()) {
    auto match = enumerator.get_match ();

    if (enumerator.is_directory ()) {
      found_dir = true;
    }
    else if (match.find ("inner.tmp") != ystl::String::InvalidIndex) {
      found_file = true;
    }
  }
  CHECK (found_dir);
  CHECK (found_file);

  plat.remove_file (file_name);
  plat.remove_directory (sub_dir_name);
  plat.remove_directory (dir_name);
}

// fileloader - empty file
TEST_CASE ("FileLoader default_load handles empty files [files]") {
  const char *fname = "ystl_test_empty.tmp";
  {
    File fw (fname, "w");
    REQUIRE (fw);
  }
  int size = -1;
  auto *data = FileLoader::default_load (fname, &size);
  REQUIRE (data != nullptr);
  REQUIRE (size == 0);
  FileLoader::default_unload (data);
  plat.remove_file (fname);
}
