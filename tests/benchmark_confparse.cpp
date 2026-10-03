// benchmark_confparse.cpp - benchmarks for the conf parser/serializer
#include <ystl/ystl.h>
#include <ystl/test.h>

using namespace ystl;

// synthetic config resembling real bot config with sections and lists
static String make_sample_config (size_t sections) {
  String out {};

  out += "// generated benchmark config\n";

  for (size_t i = 0; i < sections; ++i) {
    out += "\nSection_";
    out.appendf ("%zu", i);
    out += " {\n";
    out += "   // scalar block\n";
    out += "   aim_fov = 5.5\n";
    out += "   aim_enabled = yes\n";
    out += "   nickname = \"bot with spaces\"\n";
    out += "   weapons = 1, 2, 3, 4, 5, 6, 7, 8\n";
    out += "   pathfinder = on\n";

    out += "   Nested = {\n";
    out += "      key_a = value_a\n";
    out += "      key_b = 123456\n";
    out += "      key_c = 3.14159\n";
    out += "      items = one two three four five\n";
    out += "   }\n";

    out += "   raw Script {\n";
    out += "      line one of verbatim text\n";
    out += "      line two ; with comment chars\n";
    out += "   }\n";
    out += "}\n";
  }
  return out;
}

static const auto kSample = make_sample_config (256);

TEST_CASE ("ConfParser benchmark [benchmark][confparse]") {
  BENCHMARK ("parse 256 sections") {
    ConfParser parser;

    parser.parse (kSample);
    ystl::benchmark::deoptimize_value (parser.document ().size ());
  }
  BENCHMARK_END;

  BENCHMARK_ADVANCED ("parse + writer roundtrip") {
    meter.measure ([] {
      ConfParser parser;

      parser.parse (kSample);
      auto text = ConfWriter::write (parser.document ());
      return text.size ();
    });
  }
  BENCHMARK_ADVANCED_END;

  ConfParser parser;

  REQUIRE (parser.parse (kSample));

  BENCHMARK_ADVANCED ("document lookups") {
    const auto &doc = parser.document ();

    meter.measure ([&doc] {
      size_t hits = 0;

      for (size_t i = 0; i < 256; ++i) {
        String path {};

        path += "Section_";
        path.appendf ("%zu", i);

        const auto *section = doc.find (path);

        if (section) {
          hits += section->get_bool ("aim_enabled") ? 1u : 0u;
          hits += section->get_int ("Nested.key_b") / 123456u;
          hits += section->find ("weapons") != nullptr ? 1u : 0u;
        }
      }
      return hits;
    });
  }
  BENCHMARK_ADVANCED_END;
}
