// test_string.cpp - tests for ystl/string.h (stringref + string + strings + utf8tools)
#include <ystl/ystl.h>
#include <ystl/test.h>

using namespace ystl;

// stringref

TEST_CASE ("StringRef default construction is empty [stringref]") {
  StringRef sr;
  REQUIRE (sr.empty ());
  REQUIRE (sr.size () == 0u);
}

TEST_CASE ("StringRef from C-string stores correct length [stringref]") {
  StringRef sr ("hello");
  REQUIRE (sr.size () == 5u);
  REQUIRE_FALSE (sr.empty ());
}

TEST_CASE ("StringRef from C-string with explicit length [stringref]") {
  StringRef sr ("hello", 3u);
  REQUIRE (sr.size () == 3u);
}

TEST_CASE ("StringRef chars() returns the underlying pointer [stringref]") {
  StringRef sr ("world");
  REQUIRE (sr.chars () != nullptr);
  REQUIRE (sr.chars ()[0] == 'w');
}

TEST_CASE ("StringRef operator[] accesses characters [stringref]") {
  StringRef sr ("abc");
  REQUIRE (sr[0] == 'a');
  REQUIRE (sr[2] == 'c');
}

TEST_CASE ("StringRef equality operators [stringref]") {
  StringRef a ("foo");
  StringRef b ("foo");
  StringRef c ("bar");

  REQUIRE (a == b);
  REQUIRE_FALSE (a == c);
  REQUIRE (a != c);
  REQUIRE_FALSE (a != b);
  REQUIRE (a == "foo");
  REQUIRE (a != "bar");
}

TEST_CASE ("StringRef equals method [stringref]") {
  StringRef a ("test");
  REQUIRE (a.equals ("test"));
  REQUIRE_FALSE (a.equals ("other"));
}

// hash
TEST_CASE ("StringRef hash is consistent for same string [stringref]") {
  StringRef a ("key");
  StringRef b ("key");
  REQUIRE (a.hash () == b.hash ());
}

TEST_CASE ("StringRef hash differs for different strings [stringref]") {
  REQUIRE (StringRef ("abc").hash () != StringRef ("xyz").hash ());
}

TEST_CASE ("StringRef fnv1a32 static method produces known hash [stringref]") {
  // fnv-1a of empty string == basis 0x811c9dc5
  REQUIRE (StringRef::fnv1a32 ("") == 0x811c9dc5u);
}

// startswith / endswith /
TEST_CASE ("StringRef startsWith [stringref]") {
  StringRef sr ("hello world");
  REQUIRE (sr.starts_with ("hello"));
  REQUIRE_FALSE (sr.starts_with ("world"));
  REQUIRE (sr.starts_with (""));
}

TEST_CASE ("StringRef endsWith [stringref]") {
  StringRef sr ("hello world");
  REQUIRE (sr.ends_with ("world"));
  REQUIRE_FALSE (sr.ends_with ("hello"));
}

TEST_CASE ("StringRef contains [stringref]") {
  StringRef sr ("the quick brown fox");
  REQUIRE (sr.contains ("quick"));
  REQUIRE (sr.contains ("fox"));
  REQUIRE_FALSE (sr.contains ("cat"));
}

// find / rfind
TEST_CASE ("StringRef find by char [stringref]") {
  StringRef sr ("abcabc");
  REQUIRE (sr.find ('a') == 0u);
  REQUIRE (sr.find ('c') == 2u);
  REQUIRE (sr.find ('z') == StringRef::InvalidIndex);
  REQUIRE (sr.find ('a', 1u) == 3u);
}

TEST_CASE ("StringRef find by substring [stringref]") {
  StringRef sr ("hello world hello");
  REQUIRE (sr.find ("hello") == 0u);
  REQUIRE (sr.find ("world") == 6u);
  REQUIRE (sr.find ("xyz") == StringRef::InvalidIndex);
}

TEST_CASE ("StringRef rfind by char [stringref]") {
  StringRef sr ("abcabc");
  REQUIRE (sr.rfind ('c') == 5u);
  REQUIRE (sr.rfind ('a') == 3u);
}

TEST_CASE ("StringRef rfind finds char at index 0 [stringref]") {
  // regression: old loop started at length_ and stopped before i==0
  StringRef sr ("xyz");
  REQUIRE (sr.rfind ('x') == 0u);
  REQUIRE (sr.rfind ('z') == 2u);
  REQUIRE (sr.rfind ('q') == StringRef::InvalidIndex);
}

TEST_CASE ("StringRef rfind by substring [stringref]") {
  StringRef sr ("abcabc");
  REQUIRE (sr.rfind ("abc") == 3u);
  REQUIRE (sr.rfind ("xyz") == StringRef::InvalidIndex);
}

// findfirstof /
TEST_CASE ("StringRef findFirstOf [stringref]") {
  StringRef sr ("hello");
  REQUIRE (sr.find_first_of ("aeiou") == 1u); // 'e'
  REQUIRE (sr.find_first_of ("xyz") == StringRef::InvalidIndex);
}

TEST_CASE ("StringRef findLastOf [stringref]") {
  StringRef sr ("hello");
  REQUIRE (sr.find_last_of ("aeiou") == 4u); // 'o'
}

TEST_CASE ("StringRef findLastOf finds match at index 0 [stringref]") {
  // regression: old loop stopped before i==0
  StringRef sr ("hello");
  REQUIRE (sr.find_last_of ("h") == 0u);
  REQUIRE (sr.find_last_of ("xyz") == StringRef::InvalidIndex);
}

TEST_CASE ("StringRef findFirstNotOf [stringref]") {
  StringRef sr ("aabbcc");
  REQUIRE (sr.find_first_not_of ("ab") == 4u); // first 'c'
}

TEST_CASE ("StringRef findLastNotOf [stringref]") {
  StringRef sr ("aabbcc");
  REQUIRE (sr.find_last_not_of ("c") == 3u); // last 'b'
}

TEST_CASE ("StringRef findLastNotOf finds result at index 0 [stringref]") {
  // regression: old loop stopped before i==0
  StringRef sr ("xbbb");
  REQUIRE (sr.find_last_not_of ("b") == 0u);
}

// countchar / countstr
TEST_CASE ("StringRef countChar counts occurrences of character [stringref]") {
  StringRef sr ("banana");
  REQUIRE (sr.count_char ('a') == 3u);
  REQUIRE (sr.count_char ('b') == 1u);
  REQUIRE (sr.count_char ('z') == 0u);
}

TEST_CASE ("StringRef countStr counts occurrences of substring [stringref]") {
  StringRef sr ("abababab");
  REQUIRE (sr.count_str ("ab") == 4u);
  REQUIRE (sr.count_str ("xyz") == 0u);
}

// substr
TEST_CASE ("StringRef substr extracts a sub-range [stringref]") {
  StringRef sr ("hello world");
  StringRef sub = sr.substr (6u, 5u);
  REQUIRE (sub == "world");
  REQUIRE (sub.size () == 5u);
}

TEST_CASE ("StringRef substr without count takes rest of string [stringref]") {
  StringRef sr ("hello world");
  StringRef sub = sr.substr (6u);
  REQUIRE (sub == "world");
}

// split
TEST_CASE ("StringRef split by delimiter [stringref]") {
  StringRef sr ("a,b,c,d");
  // use string as the split result type for reliable comparison
  auto parts = sr.split<String> (",");
  REQUIRE (parts.size () == 4u);
  REQUIRE (parts[0] == "a");
  REQUIRE (parts[1] == "b");
  REQUIRE (parts[2] == "c");
  REQUIRE (parts[3] == "d");
}

TEST_CASE ("StringRef split by multi-char delimiter [stringref]") {
  // regression: old code did ++pos (1 byte) instead of pos += delim.length()
  StringRef sr ("a::b::c");
  auto parts = sr.split<String> ("::");
  REQUIRE (parts.size () == 3u);
  REQUIRE (parts[0] == "a");
  REQUIRE (parts[1] == "b");
  REQUIRE (parts[2] == "c");
}

TEST_CASE ("StringRef split with no match returns whole string [stringref]") {
  StringRef sr ("hello");
  auto parts = sr.split<String> (",");
  REQUIRE (parts.size () == 1u);
  REQUIRE (parts[0] == "hello");
}

TEST_CASE ("StringRef split by max length [stringref]") {
  StringRef sr ("abcdef");
  auto parts = sr.split<String> (2u);
  REQUIRE (parts.size () == 3u);
  REQUIRE (parts[0] == "ab");
  REQUIRE (parts[1] == "cd");
  REQUIRE (parts[2] == "ef");
}

// as<float> / as<int>
TEST_CASE ("StringRef as<int> converts numeric string [stringref]") {
  StringRef sr ("123");
  REQUIRE (sr.as<int> () == 123);

  StringRef neg ("-42");
  REQUIRE (neg.as<int> () == -42);
}

TEST_CASE ("StringRef as<float> converts float string [stringref]") {
  StringRef sr ("3.14");
  REQUIRE (sr.as<float> () == Approx (3.14f).epsilon (0.01f));
}

// begin / end
TEST_CASE ("StringRef range-based for loop iterates characters [stringref]") {
  StringRef sr ("abc");
  int count = 0;
  for (char c : sr) {
    (void)c;
    ++count;
  }
  REQUIRE (count == 3);
}

// string

TEST_CASE ("String default construction is empty [string]") {
  String s;
  REQUIRE (s.empty ());
  REQUIRE (s.size () == 0u);
}

TEST_CASE ("String construction from C-string [string]") {
  String s ("hello");
  REQUIRE (s.size () == 5u);
  REQUIRE (s == "hello");
}

TEST_CASE ("String construction from char [string]") {
  String s ('X');
  REQUIRE (s.size () == 1u);
  REQUIRE (s[0] == 'X');
}

TEST_CASE ("String copy construction [string]") {
  String a ("world");
  String b (a);
  REQUIRE (b == "world");
}

TEST_CASE ("String move construction transfers ownership [string]") {
  String a ("transfer");
  String b (ystl::move (a));
  REQUIRE (b == "transfer");
  REQUIRE (a.empty ());
}

// assign
TEST_CASE ("String assign from C-string [string]") {
  String s;
  s.assign ("assigned");
  REQUIRE (s == "assigned");
}

TEST_CASE ("String assign from char [string]") {
  String s;
  s.assign ('Z');
  REQUIRE (s.size () == 1u);
  REQUIRE (s[0] == 'Z');
}

// append / operator+=
TEST_CASE ("String append builds string incrementally [string]") {
  String s ("foo");
  s.append ("bar");
  REQUIRE (s == "foobar");
  REQUIRE (s.size () == 6u);
}

TEST_CASE ("String append char [string]") {
  String s ("ab");
  s.append ('c');
  REQUIRE (s == "abc");
}

TEST_CASE ("String operator+= appends string [string]") {
  String s ("hello");
  s += " world";
  REQUIRE (s == "hello world");
}

// assignf / appendf
TEST_CASE ("String assignf formats a string [string]") {
  String s;
  s.assignf ("value=%d", 42);
  REQUIRE (s == "value=42");
}

TEST_CASE ("String appendf appends a formatted string [string]") {
  String s ("x=");
  s.appendf ("%d", 7);
  REQUIRE (s == "x=7");
}

// operator+
TEST_CASE ("String operator+ concatenates strings [string]") {
  String a ("hello");
  String b (" world");
  String c = a + b;
  REQUIRE (c == "hello world");

  String d = a + " again";
  REQUIRE (d == "hello again");
}

// at / operator[]
TEST_CASE ("String at and operator[] access characters [string]") {
  String s ("abcde");
  REQUIRE (s.at (0) == 'a');
  REQUIRE (s[4] == 'e');

  s.at (0) = 'A';
  REQUIRE (s[0] == 'A');
}

// insert / erase
TEST_CASE ("String insert inserts substring at index [string]") {
  String s ("helo");
  REQUIRE (s.insert (3u, "l"));
  REQUIRE (s == "hello");
}

TEST_CASE ("String insert past end appends [string]") {
  String s ("hello");
  REQUIRE (s.insert (100u, " world"));
  REQUIRE (s == "hello world");
}

TEST_CASE ("String erase removes characters [string]") {
  String s ("hello world");
  REQUIRE (s.erase (5u, 6u));
  REQUIRE (s == "hello");
}

// replace
TEST_CASE ("String replace substitutes all occurrences [string]") {
  String s ("aababaa");
  size_t n = s.replace ("a", "x");
  REQUIRE (n > 0u);
  REQUIRE (s == "xxbxbxx");
}

// lowercase / uppercase
TEST_CASE ("String lowercase converts to lower case [string]") {
  String s ("HELLO World 123");
  s.lowercase ();
  REQUIRE (s == "hello world 123");
}

TEST_CASE ("String uppercase converts to upper case [string]") {
  String s ("hello world");
  s.uppercase ();
  REQUIRE (s == "HELLO WORLD");
}

// ltrim / rtrim / trim
TEST_CASE ("String ltrim strips leading whitespace [string]") {
  String s ("  \t hello");
  s.ltrim ();
  REQUIRE (s == "hello");
}

TEST_CASE ("String rtrim strips trailing whitespace [string]") {
  String s ("hello   \n");
  s.rtrim ();
  REQUIRE (s == "hello");
}

TEST_CASE ("String trim strips both ends [string]") {
  String s ("  \t  hello world  \n");
  s.trim ();
  REQUIRE (s == "hello world");
}

// contains / startswith /
TEST_CASE ("String contains delegates to StringRef [string]") {
  String s ("the quick brown fox");
  REQUIRE (s.contains ("quick"));
  REQUIRE_FALSE (s.contains ("cat"));
}

TEST_CASE ("String startsWith [string]") {
  String s ("hello world");
  REQUIRE (s.starts_with ("hello"));
  REQUIRE_FALSE (s.starts_with ("world"));
}

TEST_CASE ("String endsWith [string]") {
  String s ("hello world");
  REQUIRE (s.ends_with ("world"));
  REQUIRE_FALSE (s.ends_with ("hello"));
}

TEST_CASE ("String find by char [string]") {
  String s ("abcabc");
  REQUIRE (s.find ('a') == 0u);
  REQUIRE (s.find ('z') == String::InvalidIndex);
}

TEST_CASE ("String find by substring [string]") {
  String s ("hello world");
  REQUIRE (s.find ("world") == 6u);
  REQUIRE (s.find ("xyz") == String::InvalidIndex);
}

TEST_CASE ("String rfind by char [string]") {
  String s ("abcabc");
  REQUIRE (s.rfind ('c') == 5u);
}

// countchar / countstr
TEST_CASE ("String countChar counts occurrences [string]") {
  String s ("banana");
  REQUIRE (s.count_char ('a') == 3u);
}

TEST_CASE ("String countStr counts substring occurrences [string]") {
  String s ("ababab");
  REQUIRE (s.count_str ("ab") == 3u);
}

// substr / split
TEST_CASE ("String substr returns a new String [string]") {
  String s ("hello world");
  String sub = s.substr (6u, 5u);
  REQUIRE (sub == "world");
}

TEST_CASE ("String split by delimiter [string]") {
  String s ("one,two,three");
  auto parts = s.split (",");
  REQUIRE (parts.size () == 3u);
  REQUIRE (parts[0] == "one");
  REQUIRE (parts[1] == "two");
  REQUIRE (parts[2] == "three");
}

// as<int> / as<float>
TEST_CASE ("String as<int> converts numeric string [string]") {
  String s ("789");
  REQUIRE (s.as<int> () == 789);
}

TEST_CASE ("String as<float> converts float string [string]") {
  String s ("2.5");
  REQUIRE (s.as<float> () == Approx (2.5f));
}

// hash
TEST_CASE ("String hash is consistent for same content [string]") {
  String a ("test");
  String b ("test");
  REQUIRE (a.hash () == b.hash ());
}

// clear / empty
TEST_CASE ("String clear empties the string [string]") {
  String s ("nonempty");
  s.clear ();
  REQUIRE (s.empty ());
}

// join
TEST_CASE ("String::join concatenates array with delimiter [string]") {
  Array<String> parts;
  parts.push (String ("a"));
  parts.push (String ("b"));
  parts.push (String ("c"));

  String joined = String::join (parts, ",");
  REQUIRE (joined == "a,b,c");
}

TEST_CASE ("String::join with single element has no delimiter [string]") {
  Array<String> parts;
  parts.push (String ("only"));

  String joined = String::join (parts, ",");
  REQUIRE (joined == "only");
}

TEST_CASE ("String::join with empty array returns empty [string]") {
  Array<String> parts;
  String joined = String::join (parts, ",");
  REQUIRE (joined.empty ());
}

// range-based for
TEST_CASE ("String range-based for loop iterates characters [string]") {
  String s ("hello");
  int count = 0;
  for (char c : s) {
    (void)c;
    ++count;
  }
  REQUIRE (count == 5);
}

// str accessor
TEST_CASE ("String str() returns a valid StringRef [string]") {
  String s ("ystl");
  StringRef ref = s.str ();
  REQUIRE (ref == "ystl");
  REQUIRE (ref.size () == 4u);
}

TEST_CASE ("String str() on empty String returns non-null StringRef [string]") {
  // regression empty string str must stay non-null
  String s;
  StringRef ref = s.str ();
  REQUIRE (ref.chars () != nullptr);
  REQUIRE (ref.empty ());
}

TEST_CASE ("String hash on default-constructed String does not crash [string]") {
  String s;
  REQUIRE_NOTHROW (s.hash ());
}

TEST_CASE ("String contains on default-constructed String does not crash [string]") {
  String s;
  REQUIRE_NOTHROW (s.contains ("x"));
  REQUIRE_FALSE (s.contains ("x"));
}

// strings (pool)

TEST_CASE ("Strings::is_empty detects null and empty C-strings [strings]") {
  REQUIRE (strings.is_empty (nullptr));
  REQUIRE (strings.is_empty (""));
  REQUIRE_FALSE (strings.is_empty ("x"));
}

TEST_CASE ("Strings::matches performs case-insensitive comparison [strings]") {
  REQUIRE (strings.matches ("Hello", "hello"));
  REQUIRE (strings.matches ("ABC", "abc"));
  REQUIRE_FALSE (strings.matches ("abc", "xyz"));
}

TEST_CASE ("Strings::chars returns a writeable rotating buffer [strings]") {
  char *buf = strings.chars ();
  REQUIRE (buf != nullptr);
}

TEST_CASE ("Strings::copy copies a string safely [strings]") {
  char dst[16] {};
  strings.copy (dst, "hello", sizeof (dst));
  REQUIRE (strcmp (dst, "hello") == 0);
}

TEST_CASE ("Strings::copy respects destination capacity [strings]") {
  char dst[4] {};
  strings.copy (dst, "hello", sizeof (dst));
  REQUIRE (dst[3] == '\0'); // always null-terminated
}

TEST_CASE ("Strings::concat appends source to destination [strings]") {
  char dst[32] {};
  strings.copy (dst, "hello", sizeof (dst));
  strings.concat (dst, " world", sizeof (dst));
  REQUIRE (strcmp (dst, "hello world") == 0);
}

TEST_CASE ("Strings::format formats into rotating buffer [strings]") {
  char *buf = strings.format ("num=%d", 42);
  REQUIRE (buf != nullptr);
  REQUIRE (strcmp (buf, "num=42") == 0);
}

// additional edge case tests

// stringref nullptr
TEST_CASE ("StringRef from nullptr is safe [stringref]") {
  StringRef sr (nullptr);
  REQUIRE (sr.empty ());
  REQUIRE (sr.size () == 0u);
  REQUIRE (sr.chars () != nullptr);
}

TEST_CASE ("StringRef from nullptr with length is safe [stringref]") {
  StringRef sr (nullptr, 10u);
  REQUIRE (sr.empty ());
  REQUIRE (sr.size () == 0u);
}

TEST_CASE ("StringRef equality with nullptr [stringref]") {
  StringRef sr ("");
  REQUIRE (sr == nullptr);

  StringRef sr2 ("hello");
  REQUIRE_FALSE (sr2 == nullptr);
}

// string self-assignment
TEST_CASE ("String assign from overlapping memory is safe [string]") {
  String s ("hello world");
  s.assign (s.chars () + 6, 5);
  REQUIRE (s == "world");
}

TEST_CASE ("String assign from its own substr is safe [string]") {
  String s ("prefix_suffix");
  StringRef sub = s.str ().substr (7);
  s.assign (sub.chars (), sub.size ());
  REQUIRE (s == "suffix");
}

// find/rfind edge cases
TEST_CASE ("StringRef find empty pattern returns start [stringref]") {
  StringRef sr ("hello");
  REQUIRE (sr.find ("") == 0u);
  REQUIRE (sr.find ("", 3u) == 3u);
}

TEST_CASE ("StringRef rfind empty pattern returns length [stringref]") {
  StringRef sr ("hello");
  REQUIRE (sr.rfind ("") == 5u);
}

TEST_CASE ("StringRef find with pattern longer than source fails [stringref]") {
  StringRef sr ("hi");
  REQUIRE (sr.find ("hello") == StringRef::InvalidIndex);
}

// string null-termination
TEST_CASE ("String chars returns null-terminated string [string]") {
  String s ("test");
  const char *c = s.chars ();
  REQUIRE (c[4] == '\0');
}

TEST_CASE ("String empty chars returns valid pointer [string]") {
  String s;
  REQUIRE (s.chars () != nullptr);
  REQUIRE (s.chars ()[0] == '\0');
}

// additional tests for untested methods

// string capacity methods
TEST_CASE ("String capacity returns allocated buffer size [string]") {
  String s;
  // sso: default-constructed string has inline capacity
  REQUIRE (s.capacity () >= 1u);
  s = "hello";
  REQUIRE (s.capacity () >= 5u);
}

TEST_CASE ("String reserve increases capacity when needed [string]") {
  String s ("hello");
  size_t old_capacity = s.capacity ();
  // "hello" has length 5
  s.reserve (3u);
  REQUIRE (s.capacity () == old_capacity); // no change needed

  // reserve enough to force reallocation
  s.reserve (100u); // needed = 5 + 100 + 1 = 106 > any sso capacity
  REQUIRE (s.capacity () > old_capacity);
  REQUIRE (s == "hello"); // content unchanged
}

// string constructors
TEST_CASE ("String constructor with char* and length [string]") {
  const char *data = "hello world";
  String s (data, 5u);
  REQUIRE (s == "hello");
  REQUIRE (s.size () == 5u);
}

TEST_CASE ("String constructor from StringRef [string]") {
  StringRef sr ("test string");
  String s (sr);
  REQUIRE (s == "test string");
  REQUIRE (s.size () == 11u);
}

// string assignment with
TEST_CASE ("String assign with char* and length [string]") {
  String s ("original");
  const char *data = "new content";
  s.assign (data, 3u);
  REQUIRE (s == "new");
  REQUIRE (s.size () == 3u);
}

TEST_CASE ("String assign with String and length [string]") {
  String s ("original");
  String other ("another string");
  s.assign (other, 7u);
  REQUIRE (s == "another");
  REQUIRE (s.size () == 7u);
}

TEST_CASE ("String append with char* and length [string]") {
  String s ("prefix");
  const char *data = " suffix longer";
  s.append (data, 7u);
  REQUIRE (s == "prefix suffix");
  REQUIRE (s.size () == 13u);
}

TEST_CASE ("String append with String and length [string]") {
  String s ("start");
  String other (" and more text");
  s.append (other, 9u);
  REQUIRE (s == "start and more");
  REQUIRE (s.size () == 14u);
}

// string search methods
TEST_CASE ("String findFirstOf finds any character from set [string]") {
  String s ("hello world");
  REQUIRE (s.find_first_of ("aeiou") == 1u); // 'e' at position 1
  REQUIRE (s.find_first_of ("aeiou", 2u) == 4u); // 'o' at position 4
  REQUIRE (s.find_first_of ("xyz") == String::InvalidIndex);
}

TEST_CASE ("String findLastOf finds last character from set [string]") {
  String s ("hello world");
  REQUIRE (s.find_last_of ("aeiou") == 7u); // 'o' at position 7
  REQUIRE (s.find_last_of ("xyz") == String::InvalidIndex);
}

TEST_CASE ("String findFirstNotOf finds first character not in set [string]") {
  String s ("   hello");
  REQUIRE (s.find_first_not_of (" ") == 3u); // 'h' at position 3
  // from pos 3 search hello for char not in helo
  REQUIRE (s.find_first_not_of ("helo", 3u) == String::InvalidIndex);
}

TEST_CASE ("String findLastNotOf finds last character not in set [string]") {
  String s ("hello   ");
  REQUIRE (s.find_last_not_of (" ") == 4u); // 'o' at position 4
}

// string split with
TEST_CASE ("String split with maxLength splits by length [string]") {
  String s ("abcdefghij");
  auto parts = s.split (3u); // split into chunks of max 3 chars
  REQUIRE (parts.size () == 4u); // "abc", "def", "ghi", "j"
  REQUIRE (parts[0] == "abc");
  REQUIRE (parts[1] == "def");
  REQUIRE (parts[2] == "ghi");
  REQUIRE (parts[3] == "j");
}

TEST_CASE ("String split with maxLength=1 returns single character chunks [string]") {
  String s ("abc");
  auto parts = s.split (1u);
  REQUIRE (parts.size () == 3u);
  REQUIRE (parts[0] == "a");
  REQUIRE (parts[1] == "b");
  REQUIRE (parts[2] == "c");
}

// string operators
TEST_CASE ("String assignment from char [string]") {
  String s;
  s = 'A';
  REQUIRE (s == "A");
  REQUIRE (s.size () == 1u);
}

TEST_CASE ("String operator+ with char lhs [string]") {
  String s ("world");
  String result = 'h' + s;
  REQUIRE (result == "hworld");
}

TEST_CASE ("String operator+ with const char* lhs [string]") {
  String s ("world");
  String result = "hello " + s;
  REQUIRE (result == "hello world");
}

// utf8tools methods
TEST_CASE ("Utf8Tools toUpper converts lowercase to uppercase [utf8tools]") {
  REQUIRE (utf8tools.to_upper ('a') == 'A');
  REQUIRE (utf8tools.to_upper ('z') == 'Z');
  REQUIRE (utf8tools.to_upper ('A') == 'A');
  REQUIRE (utf8tools.to_upper ('1') == '1');
}

TEST_CASE ("Utf8Tools strToUpper converts string to uppercase [utf8tools]") {
  String s ("Hello World 123");
  String upper = utf8tools.str_to_upper (s);
  REQUIRE (upper == "HELLO WORLD 123");
}

TEST_CASE ("Utf8Tools strToUpper handles empty string [utf8tools]") {
  String s;
  String upper = utf8tools.str_to_upper (s);
  REQUIRE (upper.empty ());
}

TEST_CASE ("Utf8Tools toLower converts uppercase to lowercase [utf8tools]") {
  REQUIRE (utf8tools.to_lower ('A') == 'a');
  REQUIRE (utf8tools.to_lower ('Z') == 'z');
  REQUIRE (utf8tools.to_lower ('a') == 'a');
  REQUIRE (utf8tools.to_lower ('1') == '1');
  REQUIRE (utf8tools.to_lower ('I') == 'i'); // dotted i wins over dotless ı
}

TEST_CASE ("Utf8Tools strToLower converts string to lowercase [utf8tools]") {
  String s ("Hello World 123");
  String lower = utf8tools.str_to_lower (s);
  REQUIRE (lower == "hello world 123");
}

TEST_CASE ("Utf8Tools strToLower handles empty string [utf8tools]") {
  String s;
  String lower = utf8tools.str_to_lower (s);
  REQUIRE (lower.empty ());
}

TEST_CASE ("Utf8Tools clampChunkLen keeps ascii chunks [utf8tools]") {
  String s ("Hello World");
  REQUIRE (utf8tools.clamp_chunk_len (s.str (), 0, 5) == 5);
  REQUIRE (utf8tools.clamp_chunk_len (s.str (), 6, 100) == 5);
  REQUIRE (utf8tools.clamp_chunk_len (s.str (), 11, 100) == 0);
  REQUIRE (utf8tools.clamp_chunk_len (s.str (), 0, 0) == 0);
}

TEST_CASE ("Utf8Tools clampChunkLen never cuts multibyte [utf8tools]") {
  String s ("a\xC3\xA9"
            "b"); // a + 2-byte é + b
  REQUIRE (utf8tools.clamp_chunk_len (s.str (), 0, 2) == 1); // would cut é
  REQUIRE (utf8tools.clamp_chunk_len (s.str (), 0, 3) == 3); // whole é fits
  REQUIRE (utf8tools.clamp_chunk_len (s.str (), 0, 4) == 4); // last chunk: no backup
  REQUIRE (utf8tools.clamp_chunk_len (s.str (), 1, 2) == 2); // starts at é
}

TEST_CASE ("Utf8Tools clampChunkLen always progresses on invalid input [utf8tools]") {
  String s ("\x80\x80\x80\x80\x80", 5); // bare continuations: old backup stalled on zero
  REQUIRE (utf8tools.clamp_chunk_len (s.str (), 0, 3) == 1); // backed up, still progresses
  REQUIRE (utf8tools.clamp_chunk_len (s.str (), 2, 3) == 3); // tail is returned whole
}

TEST_CASE ("Utf8Tools forEachChunk splits lossless without cutting multibyte [utf8tools]") {
  String s ("a\xC3\xA9"
            "bcdef"); // 8 bytes, é is 2 bytes

  for (size_t max_len : { size_t (1), size_t (2), size_t (3), size_t (5), size_t (100) }) {
    Array<String> chunks;
    utf8tools.for_each_chunk (s.str (), max_len, [&] (StringRef chunk) {
      chunks.push (String (chunk));
    });
    REQUIRE (!chunks.empty ());
    REQUIRE (String::join (chunks, "") == s);

    // no chunk may start with a continuation byte, unless maxLen forces 1-byte cuts
    if (max_len > 1) {
      for (const auto &chunk : chunks) {
        REQUIRE (!chunk.empty ());
        REQUIRE (!utf8tools.is_continuation (chunk[0]));
      }
    }
  }
}

TEST_CASE ("Utf8Tools forEachChunk terminates on invalid input and empty [utf8tools]") {
  String bad ("\x80\x80\x80\x80\x80", 5);
  Array<String> chunks;
  utf8tools.for_each_chunk (bad.str (), 3, [&] (StringRef chunk) {
    chunks.push (String (chunk));
  });
  REQUIRE (chunks.size () == 3); // 1 + 1 + tail, terminates instead of stalling
  REQUIRE (String::join (chunks, "") == bad);

  size_t calls = 0;
  utf8tools.for_each_chunk (StringRef (), 10, [&] (StringRef) {
    ++calls;
  });
  REQUIRE (calls == 0);
}

// strings class methods
TEST_CASE ("Strings joinPath combines paths [strings]") {
  String path = strings.join_path ("folder", "subfolder", "file.txt");
#if defined(YSTL_WINDOWS)
  REQUIRE (path == "folder\\subfolder\\file.txt");
#else
  REQUIRE (path == "folder/subfolder/file.txt");
#endif
}

TEST_CASE ("Strings format with single argument [strings]") {
  char *result = strings.format ("Number: %d", 42);
  REQUIRE (result != nullptr);
  REQUIRE (strcmp (result, "Number: 42") == 0);
}

// snprintf wrap:
TEST_CASE ("Strings format with non-terminated StringRef view [strings]") {
  String s ("add/addbot/add_ct");
  StringRef view = StringRef (s).substr (0u, 3u); // "add", not zero-terminated

  char *result = strings.format ("[%s]", view);
  REQUIRE (strcmp (result, "[add]") == 0);
}

TEST_CASE ("Strings format with multiple non-terminated views [strings]") {
  String s ("add/addbot/add_ct");
  StringRef first = StringRef (s).substr (0u, 3u); // "add"
  StringRef second = StringRef (s).substr (4u, 6u); // "addbot"

  char *result = strings.format ("%s/%s", first, second);
  REQUIRE (strcmp (result, "add/addbot") == 0);
}

TEST_CASE ("Strings format cycles scratch slots across many calls [strings]") {
  String s ("aaa/bbb");

  for (size_t i = 0; i < 20u; ++i) {
    StringRef view = StringRef (s).substr (0u, 3u); // "aaa", not zero-terminated
    String expected {};

    expected.assignf ("%zu:aaa", i);
    REQUIRE (strcmp (strings.format ("%zu:%s", i, view), expected.chars ()) == 0);
  }
}

TEST_CASE ("snprintf wrap returns proper size for non-terminated views [strings]") {
  String s ("abcdef/ghij");
  StringRef view = StringRef (s).substr (0u, 6u); // "abcdef"

  char buf[16] {};
  const auto written = fmtwrap ().exec (buf, sizeof (buf), "%s", view);

  REQUIRE (written == 6);
  REQUIRE (strcmp (buf, "abcdef") == 0);
}

TEST_CASE ("Strings assignf with oversized non-terminated view [strings]") {
  String s {};

  for (size_t i = 0; i < 100u; ++i) {
    s += "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"; // 3600 chars
  }
  StringRef view = StringRef (s).substr (0u, 3000u); // longer than the static scratch slot

  String out {};
  out.assignf ("%s", view);

  REQUIRE (out.size () == 3000u);
  REQUIRE (out.starts_with ("0123456789"));
  REQUIRE (out.substr (2988u, 12u) == "0123456789AB");
}

TEST_CASE ("Strings format keeps terminated views on fast path [strings]") {
  String s ("terminated");
  StringRef whole (s);
  StringRef literal = "literal";

  REQUIRE (strcmp (strings.format ("%s|%s", whole, literal), "terminated|literal") == 0);
  REQUIRE (whole.chars () == s.chars ()); // no copy was made
}

// stringref methods with
TEST_CASE ("StringRef findFirstOf with start parameter [stringref]") {
  StringRef sr ("hello world");
  REQUIRE (sr.find_first_of ("aeiou", 2u) == 4u); // 'o' at position 4
  REQUIRE (sr.find_first_of ("aeiou", 5u) == 7u); // 'o' at position 7
}

TEST_CASE ("StringRef findFirstNotOf with start parameter [stringref]") {
  StringRef sr ("   hello");
  REQUIRE (sr.find_first_not_of (" ", 0u) == 3u);
  // from pos 3 search hello for char not in hel
  REQUIRE (sr.find_first_not_of ("hel", 3u) == 7u); // 'o' at position 7
}

// stringref hash
TEST_CASE ("StringRef hash instance method works correctly [stringref]") {
  StringRef sr ("test");
  uint32_t h1 = sr.hash ();
  uint32_t h2 = sr.hash ();
  REQUIRE (h1 == h2);
  REQUIRE (h1 != StringRef ("other").hash ());
}

// stringref constructor
TEST_CASE ("StringRef constructed from String references correct data [stringref]") {
  String str ("hello world");
  StringRef sr (str);
  REQUIRE (sr == "hello world");
  REQUIRE (sr.size () == 11u);
  REQUIRE (sr.chars () == str.chars ());
}

// string - additional missing tests

// string move assignment
TEST_CASE ("String move assignment transfers ownership [string]") {
  String a ("source string");
  String b;
  b = ystl::move (a);
  REQUIRE (b == "source string");
  REQUIRE (a.empty ());
}

TEST_CASE ("String move assignment self-assignment is safe [string]") {
  String s ("test");
  s = ystl::move (s);
  REQUIRE (s == "test");
}

// string comparison
TEST_CASE ("String comparison with nullptr rhs [string]") {
  String s;
  REQUIRE (s == nullptr);
  REQUIRE (nullptr == s);

  String s2 ("hello");
  REQUIRE_FALSE (s2 == nullptr);
  REQUIRE_FALSE (nullptr == s2);
  REQUIRE (s2 != nullptr);
  REQUIRE (nullptr != s2);
}

// string assignf/appendf
TEST_CASE ("String assignf with multiple arguments [string]") {
  String s;
  s.assignf ("%s %d %f", "value", 42, 3.14f);
  REQUIRE (s.starts_with ("value"));
  REQUIRE (s.contains ("42"));
}

TEST_CASE ("String appendf with multiple arguments [string]") {
  String s ("prefix: ");
  s.appendf ("%s=%d, %s=%d", "a", 1, "b", 2);
  REQUIRE (s == "prefix: a=1, b=2");
}

// string replace edge
TEST_CASE ("String replace with empty needle returns 0 [string]") {
  String s ("hello");
  size_t count = s.replace ("", "x");
  REQUIRE (count == 0u);
  REQUIRE (s == "hello");
}

TEST_CASE ("String replace with self-referential source [string]") {
  String s ("abcabc");
  StringRef ref = s.str ().substr (0u, 3u); // "abc"
  s.replace (ref, "x");
  REQUIRE (s == "xx"); // "abcabc" -> "xx" (replacing both "abc" with "x")
}

TEST_CASE ("String replace with longer replacement [string]") {
  String s ("aaa");
  s.replace ("a", "xxx");
  REQUIRE (s == "xxxxxxxxx");
}

TEST_CASE ("String replace with shorter replacement [string]") {
  String s ("xxxxx");
  s.replace ("x", "a");
  REQUIRE (s == "aaaaa");
}

// string insert edge
TEST_CASE ("String insert empty string is a no-op success [string]") {
  String s ("hello");
  StringRef empty;
  REQUIRE (s.insert (0u, empty));
  REQUIRE (s == "hello");
}

TEST_CASE ("String insert at beginning [string]") {
  String s ("world");
  s.insert (0u, "hello ");
  REQUIRE (s == "hello world");
}

TEST_CASE ("String insert self-referential [string]") {
  String s ("abc");
  StringRef ref = s.str ().substr (1u, 1u); // "b"
  s.insert (0u, ref);
  REQUIRE (s == "babc");
}

// string erase edge cases
TEST_CASE ("String erase at beginning [string]") {
  String s ("hello");
  s.erase (0u, 2u);
  REQUIRE (s == "llo");
}

TEST_CASE ("String erase at end [string]") {
  String s ("hello");
  s.erase (3u, 2u);
  REQUIRE (s == "hel");
}

TEST_CASE ("String erase entire string [string]") {
  String s ("hello");
  s.erase (0u, 5u);
  REQUIRE (s.empty ());
}

TEST_CASE ("String erase with invalid index returns false [string]") {
  String s ("hello");
  REQUIRE_FALSE (s.erase (10u, 1u));
  REQUIRE (s == "hello");
}

TEST_CASE ("String erase with count past end returns false [string]") {
  String s ("hello");
  REQUIRE_FALSE (s.erase (3u, 10u));
  REQUIRE (s == "hello");
}

// string capacity more
TEST_CASE ("String capacity grows exponentially [string]") {
  String s;
  size_t prev_cap = s.capacity ();

  // force heap allocation by exceeding sso
  s.assign ("this is a very long string that exceeds small string optimization buffer");
  REQUIRE (s.capacity () > prev_cap);

  size_t cap1 = s.capacity ();
  s.append (" more content to force reallocation");
  REQUIRE (s.capacity () >= cap1);
}

// string join with start
TEST_CASE ("String join with start parameter skips initial elements [string]") {
  Array<String> parts;
  parts.push ("skip");
  parts.push ("also skip");
  parts.push ("keep1");
  parts.push ("keep2");

  String joined = String::join (parts, "-", 2u);
  REQUIRE (joined == "keep1-keep2");
}

TEST_CASE ("String join with start past end returns empty [string]") {
  Array<String> parts;
  parts.push ("only");
  String joined = String::join (parts, ",", 10u);
  REQUIRE (joined.empty ());
}

// string at bounds
TEST_CASE ("String at returns reference for modification [string]") {
  String s ("abc");
  s.at (0) = 'X';
  REQUIRE (s == "Xbc");
}

// string substr edge
TEST_CASE ("String substr with count larger than remaining [string]") {
  String s ("hello");
  String sub = s.substr (3u, 100u);
  REQUIRE (sub == "lo");
}

TEST_CASE ("String substr at end returns empty [string]") {
  String s ("hello");
  String sub = s.substr (5u);
  REQUIRE (sub.empty ());
}

// strings rotation
TEST_CASE ("Strings chars rotates buffer on each call [strings]") {
  char *buf1 = strings.chars ();
  char *buf2 = strings.chars ();
  char *buf3 = strings.chars ();

  REQUIRE (buf1 != buf2);
  REQUIRE (buf2 != buf3);

  // after 8 calls it should wrap around
  strings.chars ();
  strings.chars ();
  strings.chars ();
  strings.chars ();
  strings.chars ();
  char *buf8 = strings.chars ();

  // buf8 should be same as buf1 (rotation count is 8)
  REQUIRE (buf8 == buf1);
}

// strings copy edge cases
TEST_CASE ("Strings copy with null source [strings]") {
  char dst[16] = "original";
  strings.copy<char> (dst, static_cast<const char *> (nullptr), sizeof (dst));
  REQUIRE (dst[0] == '\0');
}

TEST_CASE ("Strings copy with zero capacity [strings]") {
  char dst[16] = "original";
  strings.copy<char> (dst, "new", 0u);
  REQUIRE (strcmp (dst, "original") == 0); // unchanged
}

// strings concat edge
TEST_CASE ("Strings concat with full destination [strings]") {
  char dst[5] = "test"; // already at capacity (4 chars + null)
  strings.concat (dst, "x", sizeof (dst));
  REQUIRE (strcmp (dst, "test") == 0); // no space to append
}

TEST_CASE ("Strings concat with zero capacity [strings]") {
  char dst[16] = "hello";
  strings.concat (dst, " world", 0u);
  REQUIRE (strcmp (dst, "hello") == 0); // unchanged
}

// utf8tools edge cases
TEST_CASE ("Utf8Tools toUpper with special characters [utf8tools]") {
  REQUIRE (utf8tools.to_upper ('!') == '!');
  REQUIRE (utf8tools.to_upper ('1') == '1');
  REQUIRE (utf8tools.to_upper (' ') == ' ');
}

TEST_CASE ("Utf8Tools strToUpper with already uppercase [utf8tools]") {
  String s ("ALREADY UPPER");
  String upper = utf8tools.str_to_upper (s);
  REQUIRE (upper == "ALREADY UPPER");
}

TEST_CASE ("Utf8Tools strToUpper with mixed case [utf8tools]") {
  String s ("HeLLo WoRLd");
  String upper = utf8tools.str_to_upper (s);
  REQUIRE (upper == "HELLO WORLD");
}

// stringref three-way
TEST_CASE ("StringRef operator<=> orders lexicographically [string]") {
  StringRef a ("apple");
  StringRef b ("banana");
  StringRef a2 ("apple");

  REQUIRE (a == a2);
  REQUIRE (a < b);
  REQUIRE (b > a);
  REQUIRE (a <= a2);
  REQUIRE (a >= a2);
  REQUIRE (a != b);
}

TEST_CASE ("StringRef operator<=> treats shorter prefix as less [string]") {
  StringRef prefix ("app");
  StringRef full ("apple");

  REQUIRE (prefix < full);
  REQUIRE (full > prefix);
  REQUIRE (prefix <= full);
}

TEST_CASE ("StringRef operator<=> with empty strings [string]") {
  StringRef empty ("");
  StringRef something ("a");

  REQUIRE (empty < something);
  REQUIRE (empty == StringRef (""));
}

TEST_CASE ("Strings concat with null source keeps dst [string]") {
  char dst[64] = "keep";
  REQUIRE (strings.concat (dst, static_cast<const char *> (nullptr), sizeof (dst)) == dst);
  REQUIRE (strcmp (dst, "keep") == 0);
}

TEST_CASE ("Strings format with many views keeps every slot [string]") {
  const char letters[] = "abcdefghijklmnop";
  StringRef views[16];

  for (int i = 0; i < 16; ++i) {
    views[i] = StringRef (letters + i, 1);
  }

  char *out = strings.format ("%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s", views[0], views[1], views[2], views[3], views[4], views[5], views[6], views[7],
    views[8], views[9], views[10], views[11], views[12], views[13], views[14], views[15]);

  REQUIRE (strcmp (out, letters) == 0);
}
