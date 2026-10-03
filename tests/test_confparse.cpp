// test_confparse.cpp – tests for ystl/confparse.h universal config parser
#include <ystl/ystl.h>
#include <ystl/test.h>

using ystl::ConfNode;
using ystl::ConfParser;

// scalar parsing
TEST_CASE ("parses flat key = value pairs [confparse]") {
  ConfParser parser;

  REQUIRE (parser.parse ("key = value\nother = 42\n"));
  const auto &doc = parser.document ();

  REQUIRE (doc.size () == 2);
  REQUIRE (doc.find ("key") != nullptr);
  CHECK (doc.find ("key")->value () == "value");
  CHECK (doc.find ("key")->is_scalar ());
  CHECK (doc.find ("other")->as_int () == 42);
  CHECK (doc["other"] != nullptr);
  CHECK (doc["missing"] == nullptr);
}

TEST_CASE ("scalar values trim whitespace and strip inline comments [confparse]") {
  ConfParser parser;

  REQUIRE (parser.parse ("a = value   \nb = value ; comment\nc = value # comment\nd = value // comment\n"));
  const auto &doc = parser.document ();

  CHECK (doc.find ("a")->value () == "value");
  CHECK (doc.find ("b")->value () == "value");
  CHECK (doc.find ("c")->value () == "value");
  CHECK (doc.find ("d")->value () == "value");
}

TEST_CASE ("inline comment chars not preceded by space are kept [confparse]") {
  ConfParser parser;

  REQUIRE (parser.parse ("a = value;notcomment\nb = value#notcomment\n"));
  const auto &doc = parser.document ();

  CHECK (doc.find ("a")->value () == "value;notcomment");
  CHECK (doc.find ("b")->value () == "value#notcomment");
}

TEST_CASE ("quoted values preserve spaces and support escapes [confparse]") {
  ConfParser parser;

  REQUIRE (parser.parse ("a = \"hello world\"\nb = \"with \\\\ escape and \\\" quote\"\n"));
  const auto &doc = parser.document ();

  CHECK (doc.find ("a")->value () == "hello world");
  CHECK (doc.find ("b")->value () == "with \\ escape and \" quote");
}

// block parsing
TEST_CASE ("parses nested blocks [confparse]") {
  ConfParser parser;
  const auto *text = "Section {\n"
                     "   key = value\n"
                     "\n"
                     "   Nested = {\n"
                     "      inner = 1\n"
                     "   }\n"
                     "}\n";

  REQUIRE (parser.parse (text));
  const auto &doc = parser.document ();

  REQUIRE (doc.size () == 1);
  auto *section = doc.find ("Section");

  REQUIRE (section != nullptr);
  CHECK (section->is_block ());
  CHECK (section->find ("key")->value () == "value");

  auto *nested = section->find ("Nested");

  REQUIRE (nested != nullptr);
  CHECK (nested->is_block ());
  CHECK (nested->find ("inner")->as_int () == 1);
  CHECK (doc.get ("Section.Nested.inner")->as_int () == 1);
  CHECK (doc.get_int ("Section.Nested.inner", -1) == 1); // dotted path access
  CHECK (doc.get_int ("Section.Nested.missing", -1) == -1); // missing path member
}

TEST_CASE ("block requires opening brace on the same line [confparse]") {
  ConfParser parser;

  // bare name followed by newline is a list item, not a block header
  REQUIRE (parser.parse ("Section { key = value }\n"));
  CHECK (parser.document ().find ("Section")->find ("key")->value () == "value");
}

TEST_CASE ("empty blocks are preserved as blocks [confparse]") {
  ConfParser parser;

  REQUIRE (parser.parse ("empty {}\nscalar = 1\n"));
  const auto &doc = parser.document ();

  CHECK (doc.find ("empty")->is_block ());
  CHECK (doc.find ("empty")->size () == 0);
  CHECK (doc.find ("scalar")->is_scalar ());
}

TEST_CASE ("relaxed bare lines become anonymous list items [confparse]") {
  ConfParser parser;

  REQUIRE (parser.parse ("Avatars {\n   76561198007764214\n   76561198282309907\n}\n"));
  auto *root = parser.document ().find ("Avatars");

  REQUIRE (root != nullptr);
  REQUIRE (root->size () == 2);
  CHECK (root->at (0)->is_item ());
  CHECK (root->at (0)->name ().empty ());
  CHECK (root->at (0)->value () == "76561198007764214");
  CHECK (root->at (1)->value () == "76561198282309907");
}

TEST_CASE ("relaxed items may be quoted and contain spaces [confparse]") {
  ConfParser parser;

  REQUIRE (parser.parse ("Logos {\n   \"{biohaz\"\n   second item here\n   \"third, with comma\"\n}\n"));
  auto *root = parser.document ().find ("Logos");

  REQUIRE (root != nullptr);
  REQUIRE (root->size () == 3);
  CHECK (root->at (0)->value () == "{biohaz");
  CHECK (root->at (1)->value () == "second item here");
  CHECK (root->at (2)->value () == "third, with comma");
}

TEST_CASE ("relaxed items mix with keys and strip inline comments [confparse]") {
  ConfParser parser;

  REQUIRE (parser.parse ("List {\n   key = value\n   item1 ; the first\n   // whole line comment\n   item2\n}\n"));
  auto *root = parser.document ().find ("List");

  REQUIRE (root != nullptr);
  REQUIRE (root->size () == 3);
  CHECK (root->at (0)->name () == "key");
  CHECK (root->at (0)->value () == "value");
  CHECK (root->at (1)->is_item ());
  CHECK (root->at (1)->value () == "item1");
  CHECK (root->at (2)->value () == "item2");
}

TEST_CASE ("bare name at root level is an item [confparse]") {
  ConfParser parser;

  REQUIRE (parser.parse ("just_a_value\n"));
  CHECK (parser.document ().size () == 1);
  CHECK (parser.document ().at (0)->is_item ());
  CHECK (parser.document ().at (0)->value () == "just_a_value");
}

TEST_CASE ("repeated keys preserve order [confparse]") {
  ConfParser parser;

  REQUIRE (parser.parse ("item = one\nitem = two\nitem = three\n"));
  const auto &doc = parser.document ();

  REQUIRE (doc.size () == 3);
  CHECK (doc.find ("item")->value () == "one"); // find returns first
  CHECK (doc.at (0)->value () == "one");
  CHECK (doc.at (1)->value () == "two");
  CHECK (doc.at (2)->value () == "three");
}

TEST_CASE ("keys may contain slashes and dashes [confparse]") {
  ConfParser parser;

  REQUIRE (parser.parse ("BotSounds {\n   \"weapons/c4_d\" {\n      radius = 2048\n   }\n   some-key = ok\n}\n"));
  auto *section = parser.document ().find ("BotSounds");

  REQUIRE (section != nullptr);
  CHECK (section->find ("weapons/c4_d")->find ("radius")->as_int () == 2048);
  CHECK (section->find ("some-key")->value () == "ok");
}

// comments
TEST_CASE ("all comment styles are supported [confparse]") {
  ConfParser parser;
  const auto *text = "// line comment\n"
                     "; semicolon comment\n"
                     "# hash comment\n"
                     "/* block\n   comment */\n"
                     "key = value\n";

  REQUIRE (parser.parse (text));
  CHECK (parser.document ().size () == 1);
  CHECK (parser.document ().find ("key")->value () == "value");
}

// typed conversions
TEST_CASE ("asInt/asFloat/asBool with defaults [confparse]") {
  ConfParser parser;

  REQUIRE (parser.parse ("i = 42\nf = 3.5\nb1 = yes\nb2 = no\nb3 = 1\nbad = notanumber\n"));
  const auto &doc = parser.document ();

  CHECK (doc.find ("i")->as_int () == 42);
  CHECK (doc.find ("i")->as_float () == 42.0f); // integral text parses fine as float
  CHECK (doc.find ("f")->as_float () == 3.5f);
  CHECK (doc.find ("f")->as_int () == 0); // trailing junk rejected
  CHECK (doc.find ("b1")->as_bool ());
  CHECK (!doc.find ("b2")->as_bool ());
  CHECK (doc.find ("b3")->as_bool ());
  CHECK (doc.find ("bad")->as_int (7) == 7);
  CHECK (doc.find ("bad")->as_float (1.5f) == 1.5f);
  CHECK (doc.find ("bad")->as_bool (true));
  CHECK (doc.get_int ("missing", -1) == -1);
  CHECK (doc.get_float ("missing", 0.5f) == 0.5f);
  CHECK (doc.get_bool ("missing", true));
  CHECK (doc.get_string ("missing", "def") == "def");
}

TEST_CASE ("asString returns default for missing values [confparse]") {
  ConfParser parser;

  REQUIRE (parser.parse ("key = actual\n"));
  CHECK (parser.document ().find ("key")->as_string ("def") == "actual");
  CHECK (parser.document ().get_string ("missing", "def") == "def");
}

TEST_CASE ("asList splits by comma or whitespace [confparse]") {
  ConfParser parser;

  REQUIRE (parser.parse ("a = 1, 2,3 , 4\nb = x y z\nc = single\n"));
  const auto &doc = parser.document ();

  auto a = doc.find ("a")->as_list ();
  REQUIRE (a.size () == 4);
  CHECK (a[0] == "1");
  CHECK (a[3] == "4");

  auto b = doc.find ("b")->as_list ();
  REQUIRE (b.size () == 3);
  CHECK (b[1] == "y");

  auto c = doc.find ("c")->as_list ();
  REQUIRE (c.size () == 1);
}

// error handling
TEST_CASE ("reports missing value [confparse]") {
  ConfParser parser;

  CHECK (!parser.parse ("key =\n"));
  CHECK (parser.error ().starts_with ("line 1:"));
}

TEST_CASE ("reports missing closing brace [confparse]") {
  ConfParser parser;

  CHECK (!parser.parse ("Section {\n   key = value\n"));
  CHECK (parser.error ().starts_with ("line 3:"));
}

TEST_CASE ("reports unexpected closing brace [confparse]") {
  ConfParser parser;

  CHECK (!parser.parse ("key = value\n}\n"));
  CHECK (parser.error ().starts_with ("line 2:"));
}

TEST_CASE ("relaxed statement without assignment is a list item [confparse]") {
  ConfParser parser;

  REQUIRE (parser.parse ("key value\n"));
  CHECK (parser.document ().at (0)->is_item ());
  CHECK (parser.document ().at (0)->value () == "key value");
}

TEST_CASE ("reports unterminated quoted value [confparse]") {
  ConfParser parser;

  CHECK (!parser.parse ("key = \"unterminated\n"));
  CHECK (parser.error ().starts_with ("line 1:"));
}

TEST_CASE ("reports deep nesting [confparse]") {
  ConfParser parser;
  ystl::String text {};

  for (size_t i = 0; i <= ConfParser::MaxDepth + 1; ++i) {
    text.appendf ("s%zu {\n", i);
  }
  CHECK (!parser.parse (text));
  CHECK (parser.error ().contains ("depth"));
}

// real-world usage
TEST_CASE ("parses yapb-like gamedef config [confparse]") {
  ConfParser parser;
  const auto *text = "// sound templates\n"
                     "BotSounds {\n"
                     "   \"weapons/c4_d\" {\n"
                     "      flags = Defuse\n"
                     "      radius = 2048.0 ; hearable far\n"
                     "      duration = 3.0\n"
                     "   }\n"
                     "   \"weapons/ric\" {\n"
                     "      flags = Ricochet, Misc\n"
                     "      radius = 1024.0\n"
                     "      duration = 1.5\n"
                     "   }\n"
                     "}\n"
                     "\n"
                     "Weapons {\n"
                     "   USP {\n"
                     "      name = weapon_usp\n"
                     "      price = 500\n"
                     "      maxClip = 12\n"
                     "      type = Pistol\n"
                     "      teamStandard = none\n"
                     "      primaryFireHold = no\n"
                     "   }\n"
                     "   Knife {\n"
                     "      name = weapon_knife\n"
                     "      type = melee\n"
                     "      primaryFireHold = yes\n"
                     "   }\n"
                     "}\n";

  REQUIRE (parser.parse (text));
  auto *sounds = parser.document ().find ("BotSounds");

  REQUIRE (sounds != nullptr);
  REQUIRE (sounds->size () == 2);
  CHECK (sounds->at (0)->name () == "weapons/c4_d");
  CHECK (sounds->at (0)->find ("flags")->value () == "Defuse");
  CHECK (sounds->at (0)->find ("radius")->as_float () == 2048.0f);
  CHECK (sounds->at (1)->name () == "weapons/ric");

  auto *usp = parser.document ().get ("Weapons.USP");

  REQUIRE (usp != nullptr);
  CHECK (usp->find ("name")->value () == "weapon_usp");
  CHECK (usp->find ("price")->as_int () == 500);
  CHECK (!usp->find ("primaryFireHold")->as_bool ());
  CHECK (!usp->find ("teamStandard")->as_bool (false));

  auto *knife = parser.document ().get ("Weapons.Knife");

  REQUIRE (knife != nullptr);
  CHECK (knife->find ("primaryFireHold")->as_bool ());
}

TEST_CASE ("single-line blocks with bare values [confparse]") {
  ConfParser parser;

  REQUIRE (parser.parse ("Section { key = value }\nother = 1\n"));
  auto *section = parser.document ().find ("Section");

  REQUIRE (section != nullptr);
  CHECK (section->find ("key")->value () == "value");
  CHECK (parser.document ().find ("other")->as_int () == 1);
}

TEST_CASE ("windows line endings are handled [confparse]") {
  ConfParser parser;

  REQUIRE (parser.parse ("Section {\r\n   key = value\r\n   other = 2\r\n}\r\nroot = 1\r\n"));
  CHECK (parser.document ().find ("Section")->find ("key")->value () == "value");
  CHECK (parser.document ().find ("root")->as_int () == 1);
}

TEST_CASE ("node movement transfers children [confparse]") {
  ConfParser parser;

  REQUIRE (parser.parse ("// leading comment\nSection { key = value }\n// trailing comment\nroot = 1\n"));
  auto node = parser.take_document ();
  auto moved = ystl::move (node);

  CHECK (moved.find ("Section")->find ("key")->value () == "value");
}

// regression: move ctor/assignment must carry comments_ and raw_ along with the rest
TEST_CASE ("node movement preserves comments and raw status [confparse]") {
  ConfParser parser;

  REQUIRE (parser.parse ("raw Data {\n   line one\n}\n"));
  auto node = parser.take_document ();
  auto moved_ctor (ystl::move (node));

  auto *raw = moved_ctor.find ("Data");
  REQUIRE (raw != nullptr);
  CHECK (raw->is_raw ());

  auto moved_assign = ConfNode {};

  moved_assign = ystl::move (moved_ctor);
  raw = moved_assign.find ("Data");
  REQUIRE (raw != nullptr);
  CHECK (raw->is_raw ());

  ConfParser cparser;

  REQUIRE (cparser.parse ("// note\nkey = value\n"));
  auto cnode = cparser.take_document ();
  auto cmoved = ConfNode {};

  cmoved = ystl::move (cnode);
  REQUIRE (cmoved.find ("key") != nullptr);
  REQUIRE (cmoved.find ("key")->comments ().size () == 1);
  CHECK (cmoved.find ("key")->comments ()[0] == "note");
}

// raw blocks

TEST_CASE ("named raw block keeps lines verbatim [confparse]") {
  ConfParser parser;

  REQUIRE (parser.parse ("raw Text {\n   hello ; not a comment\n   \"quoted\" = stuff // literal\n   a } b\n}  \n"));
  auto *text = parser.document ().find ("Text");

  REQUIRE (text != nullptr);
  REQUIRE (text->is_block ());
  REQUIRE (text->size () == 3);
  CHECK (text->at (0)->is_item ());
  CHECK (text->at (0)->value () == "hello ; not a comment");
  CHECK (text->at (1)->value () == "\"quoted\" = stuff // literal");
  CHECK (text->at (2)->value () == "a } b");
}

TEST_CASE ("anonymous raw block works [confparse]") {
  ConfParser parser;

  REQUIRE (parser.parse ("Group {\n   keys = one, two\n\n   raw {\n   line one\n\n      line two\n   }\n}\n"));
  auto *group = parser.document ().find ("Group");

  REQUIRE (group != nullptr);
  CHECK (group->find ("keys")->value () == "one, two");

  const auto *raw = group->at (1);

  REQUIRE (raw->is_block ());
  CHECK (raw->is_item () == false);
  REQUIRE (raw->size () == 2);
  CHECK (raw->at (0)->value () == "line one");
  CHECK (raw->at (1)->value () == "line two");
}

TEST_CASE ("raw block closes on indented brace only [confparse]") {
  ConfParser parser;

  REQUIRE (parser.parse ("raw T {\n   }\nkey = 1\n"));
  CHECK (parser.document ().find ("T")->size () == 0);
  CHECK (parser.document ().find ("key")->as_int () == 1);
}

TEST_CASE ("raw block does not swallow siblings [confparse]") {
  ConfParser parser;

  REQUIRE (parser.parse ("raw A { x ; y\n}\nraw B { z\n}\nlast = 1\n"));
  auto *doc = &parser.document ();

  CHECK (doc->find ("A")->at (0)->value () == "x ; y");
  CHECK (doc->find ("B")->at (0)->value () == "z");
  CHECK (doc->find ("last")->as_int () == 1);
}

TEST_CASE ("raw modifier keeps scalar and item meanings [confparse]") {
  ConfParser parser;

  REQUIRE (parser.parse ("raw = value\nraw some bare item\nother { raw = nested }\n"));
  const auto &doc = parser.document ();

  CHECK (doc.find ("raw")->is_scalar ());
  CHECK (doc.find ("raw")->value () == "value");
  CHECK (doc.size () == 3);
  CHECK (doc.at (1)->is_item ());
  CHECK (doc.at (1)->value () == "raw some bare item");
  CHECK (doc.find ("other")->find ("raw")->value () == "nested");
}

TEST_CASE ("unterminated raw block fails [confparse]") {
  ConfParser parser;

  CHECK_FALSE (parser.parse ("raw Text { line one\n"));
  CHECK (ystl::StringRef (parser.error ()).find ("raw block") != ystl::String::InvalidIndex);
}

// writer

using ystl::ConfWriter;

static void require_same_node (const ConfNode &a, const ConfNode &b) {
  CHECK (a.name () == b.name ());
  CHECK (a.value () == b.value ());
  CHECK (a.is_block () == b.is_block ());
  CHECK (a.is_raw () == b.is_raw ());
  REQUIRE (a.comments ().size () == b.comments ().size ());

  for (size_t i = 0; i < a.comments ().size (); ++i) {
    CHECK (a.comments ()[i] == b.comments ()[i]);
  }
  REQUIRE (a.size () == b.size ());

  for (size_t i = 0; i < a.size (); ++i) {
    require_same_node (*a.at (i), *b.at (i));
  }
}

TEST_CASE ("parse-write-parse roundtrip is stable [confparse]") {
  ConfParser parser;
  const auto *text = "// file header\n"
                     "; old style header\n"
                     "Section {\n"
                     "   key = plain\n"
                     "   \"quoted key\" = \"has spaces\"\n"
                     "   tricky = \"semi ; hash # brace } eq = comma , slash\"\n"
                     "   empty = \"\"\n"
                     "   multi = \"first\\nsecond\\ttabbed\"\n"
                     "\n"
                     "   // comment inside block\n"
                     "   Nested {\n"
                     "      deep = 1\n"
                     "   }\n"
                     "   raw Verbatim {\n"
                     "      keeps ; everything\n"
                     "      \"even quotes\"\n"
                     "   }\n"
                     "   raw {\n"
                     "      anonymous line\n"
                     "   }\n"
                     "   # hash comment\n"
                     "   item value here\n"
                     "}\n";

  REQUIRE (parser.parse (text));

  auto written = ConfWriter::write (parser.document ());
  ConfParser reparser;

  REQUIRE (reparser.parse (written));
  require_same_node (parser.document (), reparser.document ());

  // second roundtrip must be byte-stable
  auto written2 = ConfWriter::write (reparser.document ());

  CHECK (written == written2);
}

TEST_CASE ("comments are captured with prefixes stripped [confparse]") {
  ConfParser parser;

  REQUIRE (parser.parse ("// slash\n; semi\n# hash\nkey = value\n"));
  auto *key = parser.document ().find ("key");

  REQUIRE (key != nullptr);
  REQUIRE (key->comments ().size () == 3);
  CHECK (key->comments ()[0] == "slash");
  CHECK (key->comments ()[1] == "semi");
  CHECK (key->comments ()[2] == "hash");
}

TEST_CASE ("block comments are dropped, trailing inline comments are lost [confparse]") {
  ConfParser parser;

  REQUIRE (parser.parse ("/* gone */\nkey = value ; trailing\n"));
  auto *key = parser.document ().find ("key");

  REQUIRE (key != nullptr);
  CHECK (key->comments ().empty ());
  CHECK (key->value () == "value");
}

TEST_CASE ("writer escapes values that need quoting [confparse]") {
  ConfNode doc;

  doc.add_scalar ("spaces", "hello world");
  doc.add_scalar ("empty", "");
  doc.add_scalar ("newline", "a\nb");
  doc.add_scalar ("plain", "noquotes");

  auto text = ConfWriter::write (doc);

  CHECK (text.contains ("spaces = \"hello world\""));
  CHECK (text.contains ("empty = \"\""));
  CHECK (text.contains ("newline = \"a\\nb\""));
  CHECK (text.contains ("plain = noquotes"));

  ConfParser parser;

  REQUIRE (parser.parse (text));
  CHECK (parser.document ().find ("spaces")->value () == "hello world");
  CHECK (parser.document ().find ("empty")->value () == "");
  CHECK (parser.document ().find ("newline")->value () == "a\nb");
  CHECK (parser.document ().find ("plain")->value () == "noquotes");
}

TEST_CASE ("programmatic tree building roundtrips [confparse]") {
  ConfNode doc;
  auto &lang = doc.add_block ("Lang");

  lang.add_comment ("@package: YaPB");

  auto &entry = lang.add_block ("EntryLabel");

  entry.add_comment ("a comment for the entry");

  auto &orig = entry.add_raw_block ("Original");

  orig.add_item ("first line");
  orig.add_item ("second line");

  auto &trans = entry.add_raw_block ("Translated");

  trans.add_item ("first line");
  entry.add_scalar ("meta", "some value");

  auto text = ConfWriter::write (doc);
  ConfParser parser;

  REQUIRE (parser.parse (text));

  auto *parsed_lang = parser.document ().find ("Lang");

  REQUIRE (parsed_lang != nullptr);
  REQUIRE (parsed_lang->comments ().size () == 1);
  CHECK (parsed_lang->comments ()[0] == "@package: YaPB");

  auto *parsed_entry = parsed_lang->find ("EntryLabel");

  REQUIRE (parsed_entry != nullptr);
  CHECK (parsed_entry->comments ().size () == 1);

  auto *parsed_orig = parsed_entry->find ("Original");

  REQUIRE (parsed_orig != nullptr);
  CHECK (parsed_orig->is_raw ());
  REQUIRE (parsed_orig->size () == 2);
  CHECK (parsed_orig->at (0)->value () == "first line");
  require_same_node (doc, parser.document ());
}

// quoted value followed by block close on the same line
TEST_CASE ("quoted value keeps same-line block close [confparse]") {
  ConfParser parser;

  REQUIRE (parser.parse ("node {\nkey = \"v\" }\n"));
  const auto &doc = parser.document ();

  REQUIRE (doc.size () == 1);
  auto *node = doc.find ("node");
  REQUIRE (node != nullptr);
  REQUIRE (node->size () == 1);
  CHECK (node->find ("key")->value () == "v");
}
