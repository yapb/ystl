// test_inlinelist.cpp - tests for ystl/inlinelist.h
#include <ystl/ystl.h>
#include <ystl/test.h>

using namespace ystl;

struct Item : InlineListNode<Item> {
  int value = 0;

  Item () = default;
  explicit Item (int v) : value (v) {}
};

static void require_order (InlineList<Item> &list, const int *expected, size_t count) {
  size_t index = 0;

  for (auto &item : list) {
    REQUIRE (index < count);
    REQUIRE (item.value == expected[index]);
    ++index;
  }
  REQUIRE (index == count);
  REQUIRE (list.size () == count);
}

TEST_CASE ("InlineList default construction is empty [inlinelist]") {
  InlineList<Item> list;
  REQUIRE (list.empty ());
  REQUIRE (list.size () == 0u);
  REQUIRE (list.front () == nullptr);
  REQUIRE (list.back () == nullptr);
}

TEST_CASE ("InlineList push_back keeps insertion order [inlinelist]") {
  Item a (1), b (2), c (3);
  InlineList<Item> list;

  list.push_back (a);
  list.push_back (b);
  list.push_back (c);

  const int expected[] = { 1, 2, 3 };
  require_order (list, expected, 3);

  REQUIRE (list.front () == &a);
  REQUIRE (list.back () == &c);
}

TEST_CASE ("InlineList push_front prepends [inlinelist]") {
  Item a (1), b (2), c (3);
  InlineList<Item> list;

  list.push_front (a);
  list.push_front (b);
  list.push_front (c);

  const int expected[] = { 3, 2, 1 };
  require_order (list, expected, 3);

  REQUIRE (list.front () == &c);
  REQUIRE (list.back () == &a);
}

TEST_CASE ("InlineList unlink removes middle head and tail [inlinelist]") {
  Item a (1), b (2), c (3);
  InlineList<Item> list;

  list.push_back (a);
  list.push_back (b);
  list.push_back (c);

  list.unlink (b);

  const int first[] = { 1, 3 };
  require_order (list, first, 2);
  REQUIRE_FALSE (b.linked ());

  list.unlink (a);

  const int second[] = { 3 };
  require_order (list, second, 1);

  list.unlink (c);
  REQUIRE (list.empty ());
  REQUIRE (list.front () == nullptr);
}

TEST_CASE ("InlineList clear drops links without destroying objects [inlinelist]") {
  Item a (1), b (2);
  InlineList<Item> list;

  list.push_back (a);
  list.push_back (b);
  list.clear ();

  REQUIRE (list.empty ());
  REQUIRE_FALSE (a.linked ());
  REQUIRE_FALSE (b.linked ());
  REQUIRE (a.value == 1); // objects untouched
  REQUIRE (b.value == 2);
}

TEST_CASE ("InlineList linked tracks membership [inlinelist]") {
  Item a (1);
  InlineList<Item> list;

  REQUIRE_FALSE (a.linked ());
  list.push_back (a);
  REQUIRE (a.linked ());
  list.unlink (a);
  REQUIRE_FALSE (a.linked ());
}

TEST_CASE ("InlineList const iteration reads values [inlinelist]") {
  Item a (1), b (2);
  InlineList<Item> list;
  list.push_back (a);
  list.push_back (b);

  const auto &clist = list;
  int sum = 0;

  for (const auto &item : clist) {
    sum += item.value;
  }
  REQUIRE (sum == 3);
  REQUIRE (clist.size<int> () == 2);
}

TEST_CASE ("InlineList decrement from end reaches tail [inlinelist]") {
  Item a (1), b (2), c (3);
  InlineList<Item> list;
  list.push_back (a);
  list.push_back (b);
  list.push_back (c);

  auto it = list.end ();
  --it;
  REQUIRE ((*it).value == 3);
  --it;
  REQUIRE ((*it).value == 2);
}

TEST_CASE ("InlineList erase while iterating removes every other [inlinelist]") {
  Item items[5] = { Item (0), Item (1), Item (2), Item (3), Item (4) };
  InlineList<Item> list;

  for (auto &item : items) {
    list.push_back (item);
  }

  for (auto it = list.begin (); it != list.end ();) {
    if ((*it).value % 2 == 0) {
      it = list.erase (it);
    }
    else {
      ++it;
    }
  }

  const int expected[] = { 1, 3 };
  require_order (list, expected, 2);
}
