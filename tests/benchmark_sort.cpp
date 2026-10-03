// benchmark_sort.cpp - benchmark ystl::sort vs std::sort
#include <ystl/ystl.h>
#include <algorithm>
#include <cstdlib>
#include <vector>
#include <ystl/test.h>

using namespace ystl;

static constexpr size_t kSmallSize = 100;
static constexpr size_t kMediumSize = 1000;
static constexpr size_t kLargeSize = 10000;

TEST_CASE ("Sort benchmark - random data [benchmark][sort]") {
  SECTION ("Small (100 elements)") {
    BENCHMARK_ADVANCED ("ystl::sort") {
      std::vector<Array<int>> arrs;
      arrs.reserve (static_cast<size_t> (meter.runs ()));
      for (int r = 0; r < meter.runs (); ++r) {
        arrs.emplace_back ();
        for (size_t i = 0; i < kSmallSize; ++i) {
          arrs.back ().push (rand () % 1000);
        }
      }
      meter.measure_indexed_once ([&] (int i) {
        sort (arrs[i]);
      });
    }
    BENCHMARK_ADVANCED_END;

    BENCHMARK_ADVANCED ("std::sort") {
      std::vector<std::vector<int>> vecs (static_cast<size_t> (meter.runs ()));
      for (auto &v : vecs) {
        for (size_t i = 0; i < kSmallSize; ++i) {
          v.push_back (rand () % 1000);
        }
      }
      meter.measure_indexed_once ([&] (int i) {
        std::sort (vecs[i].begin (), vecs[i].end ());
      });
    }
    BENCHMARK_ADVANCED_END;
  }

  SECTION ("Medium (1000 elements)") {
    BENCHMARK_ADVANCED ("ystl::sort") {
      std::vector<Array<int>> arrs;
      arrs.reserve (static_cast<size_t> (meter.runs ()));
      for (int r = 0; r < meter.runs (); ++r) {
        arrs.emplace_back ();
        for (size_t i = 0; i < kMediumSize; ++i) {
          arrs.back ().push (rand () % 10000);
        }
      }
      meter.measure_indexed_once ([&] (int i) {
        sort (arrs[i]);
      });
    }
    BENCHMARK_ADVANCED_END;

    BENCHMARK_ADVANCED ("std::sort") {
      std::vector<std::vector<int>> vecs (static_cast<size_t> (meter.runs ()));
      for (auto &v : vecs) {
        for (size_t i = 0; i < kMediumSize; ++i) {
          v.push_back (rand () % 10000);
        }
      }
      meter.measure_indexed_once ([&] (int i) {
        std::sort (vecs[i].begin (), vecs[i].end ());
      });
    }
    BENCHMARK_ADVANCED_END;
  }

  SECTION ("Large (10000 elements)") {
    BENCHMARK_ADVANCED ("ystl::sort") {
      std::vector<Array<int>> arrs;
      arrs.reserve (static_cast<size_t> (meter.runs ()));
      for (int r = 0; r < meter.runs (); ++r) {
        arrs.emplace_back ();
        for (size_t i = 0; i < kLargeSize; ++i) {
          arrs.back ().push (rand () % 100000);
        }
      }
      meter.measure_indexed_once ([&] (int i) {
        sort (arrs[i]);
      });
    }
    BENCHMARK_ADVANCED_END;

    BENCHMARK_ADVANCED ("std::sort") {
      std::vector<std::vector<int>> vecs (static_cast<size_t> (meter.runs ()));
      for (auto &v : vecs) {
        for (size_t i = 0; i < kLargeSize; ++i) {
          v.push_back (rand () % 100000);
        }
      }
      meter.measure_indexed_once ([&] (int i) {
        std::sort (vecs[i].begin (), vecs[i].end ());
      });
    }
    BENCHMARK_ADVANCED_END;
  }
}

TEST_CASE ("Sort benchmark - already sorted [benchmark][sort]") {
  BENCHMARK_ADVANCED ("ystl::sort") {
    std::vector<Array<int>> arrs;
    arrs.reserve (static_cast<size_t> (meter.runs ()));
    for (int r = 0; r < meter.runs (); ++r) {
      arrs.emplace_back ();
      for (size_t i = 0; i < kMediumSize; ++i) {
        arrs.back ().push (static_cast<int> (i));
      }
    }
    meter.measure_indexed_once ([&] (int i) {
      sort (arrs[i]);
    });
  }
  BENCHMARK_ADVANCED_END;

  BENCHMARK_ADVANCED ("std::sort") {
    std::vector<std::vector<int>> vecs (static_cast<size_t> (meter.runs ()));
    for (auto &v : vecs) {
      for (size_t i = 0; i < kMediumSize; ++i) {
        v.push_back (static_cast<int> (i));
      }
    }
    meter.measure_indexed_once ([&] (int i) {
      std::sort (vecs[i].begin (), vecs[i].end ());
    });
  }
  BENCHMARK_ADVANCED_END;
}

TEST_CASE ("Sort benchmark - reverse sorted [benchmark][sort]") {
  BENCHMARK_ADVANCED ("ystl::sort") {
    std::vector<Array<int>> arrs;
    arrs.reserve (static_cast<size_t> (meter.runs ()));
    for (int r = 0; r < meter.runs (); ++r) {
      arrs.emplace_back ();
      for (size_t i = kMediumSize; i > 0; --i) {
        arrs.back ().push (static_cast<int> (i));
      }
    }
    meter.measure_indexed_once ([&] (int i) {
      sort (arrs[i]);
    });
  }
  BENCHMARK_ADVANCED_END;

  BENCHMARK_ADVANCED ("std::sort") {
    std::vector<std::vector<int>> vecs (static_cast<size_t> (meter.runs ()));
    for (auto &v : vecs) {
      for (size_t i = kMediumSize; i > 0; --i) {
        v.push_back (static_cast<int> (i));
      }
    }
    meter.measure_indexed_once ([&] (int i) {
      std::sort (vecs[i].begin (), vecs[i].end ());
    });
  }
  BENCHMARK_ADVANCED_END;
}

TEST_CASE ("Sort with custom comparator [benchmark][sort]") {
  BENCHMARK_ADVANCED ("ystl::sort with lambda") {
    std::vector<Array<int>> arrs;
    arrs.reserve (static_cast<size_t> (meter.runs ()));
    for (int r = 0; r < meter.runs (); ++r) {
      arrs.emplace_back ();
      for (size_t i = 0; i < kMediumSize; ++i) {
        arrs.back ().push (rand () % 10000);
      }
    }
    meter.measure_indexed_once ([&] (int i) {
      sort (arrs[i], [] (const int &a, const int &b) {
        return a > b; // descending
      });
    });
  }
  BENCHMARK_ADVANCED_END;

  BENCHMARK_ADVANCED ("std::sort with lambda") {
    std::vector<std::vector<int>> vecs (static_cast<size_t> (meter.runs ()));
    for (auto &v : vecs) {
      for (size_t i = 0; i < kMediumSize; ++i) {
        v.push_back (rand () % 10000);
      }
    }
    meter.measure_indexed_once ([&] (int i) {
      std::sort (vecs[i].begin (), vecs[i].end (), [] (const int &a, const int &b) {
        return a > b; // descending
      });
    });
  }
  BENCHMARK_ADVANCED_END;
}

// benchmarks for data
TEST_CASE ("Sort benchmark - all duplicates [benchmark][sort]") {
  BENCHMARK_ADVANCED ("ystl::sort (all same)") {
    std::vector<Array<int>> arrs;
    arrs.reserve (static_cast<size_t> (meter.runs ()));
    for (int r = 0; r < meter.runs (); ++r) {
      arrs.emplace_back ();
      for (size_t i = 0; i < kMediumSize; ++i) {
        arrs.back ().push (42);
      }
    }
    meter.measure_indexed_once ([&] (int i) {
      sort (arrs[i]);
    });
  }
  BENCHMARK_ADVANCED_END;

  BENCHMARK_ADVANCED ("std::sort (all same)") {
    std::vector<std::vector<int>> vecs (static_cast<size_t> (meter.runs ()));
    for (auto &v : vecs) {
      for (size_t i = 0; i < kMediumSize; ++i) {
        v.push_back (42);
      }
    }
    meter.measure_indexed_once ([&] (int i) {
      std::sort (vecs[i].begin (), vecs[i].end ());
    });
  }
  BENCHMARK_ADVANCED_END;
}

TEST_CASE ("Sort benchmark - few unique values [benchmark][sort]") {
  BENCHMARK_ADVANCED ("ystl::sort (few unique)") {
    std::vector<Array<int>> arrs;
    arrs.reserve (static_cast<size_t> (meter.runs ()));
    for (int r = 0; r < meter.runs (); ++r) {
      arrs.emplace_back ();
      for (size_t i = 0; i < kMediumSize; ++i) {
        arrs.back ().push (rand () % 10); // only 10 unique values
      }
    }
    meter.measure_indexed_once ([&] (int i) {
      sort (arrs[i]);
    });
  }
  BENCHMARK_ADVANCED_END;

  BENCHMARK_ADVANCED ("std::sort (few unique)") {
    std::vector<std::vector<int>> vecs (static_cast<size_t> (meter.runs ()));
    for (auto &v : vecs) {
      for (size_t i = 0; i < kMediumSize; ++i) {
        v.push_back (rand () % 10);
      }
    }
    meter.measure_indexed_once ([&] (int i) {
      std::sort (vecs[i].begin (), vecs[i].end ());
    });
  }
  BENCHMARK_ADVANCED_END;
}
