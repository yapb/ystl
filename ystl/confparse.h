// SPDX-License-Identifier: Unlicense

#pragma once

// lightweight universal config format parser ("conf" format)
//
// grammar:
//   document  := { statement }
//   statement := name ( block | '=' ( block | value ) | bare-value )
//   block     := '{' { statement } '}'
//
// example:
//   // comment, also ; and # and block comments
//   Section {
//      key = value                  ; scalar value (rest of the line)
//      list = one, two, three       ; comma or whitespace separated list
//      "key with spaces" = "quoted value"
//
//      Nested = {                   ; '=' before '{' is optional
//         key = value
//      }
//
//      raw Verbatim {               ; raw block: no comments, quotes or '=' inside,
//         any ; text // line        ; every non-empty line becomes an anonymous item,
//         "even quoted" = stuff     ; closed by a line that is exactly '}'
//      }
//
//      raw {                        ; anonymous raw block
//         just a line
//      }
//   }
//
// relaxed list items: a bare line without '=' or '{' becomes an anonymous
// value item (name is empty); '{' must be on the same line with its name:
//
//   Avatars {
//      76561198007764214        // bare list item
//      "quoted item value"      // quoted list item
//   }
//
// repeated keys are allowed and preserved in order (iterate children())
// node is either a scalar (key = value) or a block (key { ... })
//
// the parser captures full-line comments per node; ConfWriter serializes a
// ConfNode tree back to text (roundtrip: parse -> write -> parse is stable,
// see ConfWriter notes for what is preserved)

#include <limits.h>
#include <stdlib.h>

#include <ystl/array.h>
#include <ystl/string.h>
#include <ystl/tokenizer.h>
#include <ystl/uniqueptr.h>

namespace ystl {

class ConfNode;

// resolves an @include target to its text; returns empty to report a missing file.
// 'from' is the path of the including file, so relative includes resolve against it.
using ConfIncludeLoader = String (*) (StringRef path, StringRef from, void *user);

// bump-allocator pool backing confnode trees, nodes live inside large chunks
class ConfNodePool final : public NonCopyable {
public:
  static constexpr size_t kChunkSize = 4096;

  static_assert (kChunkSize % sizeof (void *) == 0, "chunk size must be a whole number of pointer slots");

private:
  struct Chunk {
    UniquePtr<Chunk> next {};
    size_t used = 0;
    void *bytes[kChunkSize / sizeof (void *)]; // pointer-aligned node storage
  };

  UniquePtr<Chunk> head_ {};
  size_t count_ = 0;

public:
  ConfNodePool () = default;

  ~ConfNodePool ();

  // destroys all pooled nodes and releases the chunks
  void clear ();

  // default-constructs a node inside the pool
  ConfNode *create ();

  size_t count () const {
    return count_;
  }

private:
  void grow ();
};

// single config document node: named scalar or named block of child nodes
class ConfNode final : public NonCopyable {
  friend class ConfParser;
  friend class ConfNodePool;

private:
  String name_ {};
  String value_ {};
  Array<ConfNode *> children_ {}; // nodes themselves are owned by the backing pool
  Array<String> comments_ {}; // full-line comments preceding the node (prefix stripped)
  bool block_ = false;
  bool raw_ = false;
  UniquePtr<ConfNodePool> owned_pool_ {}; // node pool, owned by the tree root only
  ConfNodePool *pool_ = nullptr; // pool backing this tree (set for every node)

public:
  ConfNode () = default;

  ConfNode (ConfNode &&rhs) noexcept :
    name_ (ystl::move (rhs.name_)), value_ (ystl::move (rhs.value_)), children_ (ystl::move (rhs.children_)),
    comments_ (ystl::move (rhs.comments_)), block_ (rhs.block_), raw_ (rhs.raw_), owned_pool_ (ystl::move (rhs.owned_pool_)), pool_ (rhs.pool_) {
    rhs.pool_ = nullptr;
  }

  ConfNode &operator= (ConfNode &&rhs) noexcept {
    if (this != &rhs) {
      name_ = ystl::move (rhs.name_);
      value_ = ystl::move (rhs.value_);
      children_ = ystl::move (rhs.children_);
      comments_ = ystl::move (rhs.comments_);
      block_ = rhs.block_;
      raw_ = rhs.raw_;
      owned_pool_ = ystl::move (rhs.owned_pool_);
      pool_ = rhs.pool_;
      rhs.pool_ = nullptr;
    }
    return *this;
  }

public:
  StringRef name () const {
    return name_;
  }

  // scalar payload; empty for blocks
  StringRef value () const {
    return value_;
  }

  bool is_block () const {
    return block_;
  }

  bool is_scalar () const {
    return !block_;
  }

  // verbatim block ('raw name {') - children are plain lines, no quoting rules
  bool is_raw () const {
    return raw_;
  }

  void set_raw (bool raw) {
    raw_ = raw;
  }

  // full-line comments preceding this node ('//', ';', '#' prefixes are stripped, block comments are dropped)
  const Array<String> &comments () const {
    return comments_;
  }

  void add_comment (StringRef text) {
    comments_.push (text);
  }

  // anonymous list item (bare value line without a key)
  bool is_item () const {
    return !block_ && name_.empty ();
  }

  // direct child nodes in document order
  const Array<ConfNode *> &children () const {
    return children_;
  }

  size_t size () const {
    return children_.size ();
  }

  // child node by index (document order)
  const ConfNode *at (size_t index) const {
    return children_[index];
  }

public:
  // programmatic tree building, usable together with confwriter
  ConfNode &add_block (StringRef name) {
    auto *node = ensure_pool ()->create ();

    node->name_ = name;
    node->block_ = true;
    return add_child (node);
  }

  ConfNode &add_raw_block (StringRef name) {
    auto &node = add_block (name);
    node.set_raw (true);
    return node;
  }

  ConfNode &add_scalar (StringRef name, StringRef value) {
    auto *node = ensure_pool ()->create ();

    node->name_ = name;
    node->value_ = value;
    return add_child (node);
  }

  // anonymous list item (bare value line)
  ConfNode &add_item (StringRef value) {
    auto *node = ensure_pool ()->create ();

    node->value_ = value;
    return add_child (node);
  }

private:
  // returns the pool backing this tree, creating one on first use (owned by the tree root)
  ConfNodePool *ensure_pool () {
    if (!pool_) {
      owned_pool_ = make_unique<ConfNodePool> ();
      pool_ = owned_pool_.get ();
    }
    return pool_;
  }

  ConfNode &add_child (ConfNode *node) {
    children_.push (node);
    return *children_[children_.size () - 1];
  }

public:
  // finds first direct child by name, nullptr if not found
  const ConfNode *find (StringRef key) const {
    for (const auto *child : children_) {
      if (child->name_ == key) {
        return child;
      }
    }
    return nullptr;
  }

  // mutable counterpart of find (), for tree building
  ConfNode *find (StringRef key) {
    for (auto *child : children_) {
      if (child->name_ == key) {
        return child;
      }
    }
    return nullptr;
  }

  const ConfNode *operator[] (StringRef key) const {
    return find (key);
  }

  // resolves key as a direct child name, or as a dotted path when it contains '.'
  const ConfNode *resolve (StringRef key) const {
    return key.find (".") == String::InvalidIndex ? find (key) : get (key);
  }

  // null-safe typed accessors: return def when key is missing or malformed
  int32_t get_int (StringRef key, int32_t def = 0) const {
    const auto *node = resolve (key);
    return node ? node->as_int (def) : def;
  }

  float get_float (StringRef key, float def = 0.0f) const {
    const auto *node = resolve (key);
    return node ? node->as_float (def) : def;
  }

  bool get_bool (StringRef key, bool def = false) const {
    const auto *node = resolve (key);
    return node ? node->as_bool (def) : def;
  }

  StringRef get_string (StringRef key, StringRef def = "") const {
    const auto *node = resolve (key);
    return node ? node->as_string (def) : def;
  }

  // walks dotted path like "section.nested.key"
  const ConfNode *get (StringRef path) const {
    auto node = this;
    size_t start = 0;

    for (;;) {
      const auto dot = path.find (".", start);
      const auto part = dot == String::InvalidIndex ? path.substr (start) : path.substr (start, dot - start);

      node = node->find (part);

      if (!node || dot == String::InvalidIndex) {
        return node;
      }
      start = dot + 1;
    }
  }

public:
  // scalar conversions with fallback to default on absence/malformation
  int32_t as_int (int32_t def = 0) const {
    char *end = nullptr;
    const auto value = strtol (value_.chars (), &end, 10);

    if (!end || end == value_.chars () || *end != kNullChar || value < numeric_limits<int32_t>::min () ||
        value > numeric_limits<int32_t>::max ()) [[unlikely]] {
      return def;
    }
    return static_cast<int32_t> (value);
  }

  float as_float (float def = 0.0f) const {
    char *end = nullptr;
    const auto value = strtod (value_.chars (), &end);

    if (!end || end == value_.chars () || *end != kNullChar) {
      return def;
    }
    return static_cast<float> (value);
  }

  bool as_bool (bool def = false) const {
    if (value_ == "1") {
      return true;
    }
    if (value_ == "0") {
      return false;
    }
    if (equals_no_case (value_, "yes") || equals_no_case (value_, "true") || equals_no_case (value_, "on") ||
        equals_no_case (value_, "enable") || equals_no_case (value_, "enabled")) {
      return true;
    }
    if (equals_no_case (value_, "no") || equals_no_case (value_, "false") || equals_no_case (value_, "off") ||
        equals_no_case (value_, "disable") || equals_no_case (value_, "disabled")) {
      return false;
    }
    return def;
  }

  StringRef as_string (StringRef def = "") const {
    return value_.empty () ? def : StringRef (value_);
  }

  // splits scalar into list: comma-separated, or whitespace-separated when no comma present
  Array<String> as_list () const {
    Array<String> out {};
    const auto has_comma = value_.find (",") != String::InvalidIndex;

    if (has_comma) {
      for (auto &token : value_.split (",")) {
        token.trim ();

        if (!token.empty ()) {
          out.push (ystl::move (token));
        }
      }
      return out;
    }

    for (auto &part : value_.split (" ")) {
      for (auto &token : part.split ("\t")) {
        token.trim ();

        if (!token.empty ()) {
          out.push (ystl::move (token));
        }
      }
    }
    return out;
  }

private:
  // allocation-free ascii case-insensitive comparison
  static bool equals_no_case (StringRef lhs, StringRef rhs) {
    return lhs.equals_no_case (rhs);
  }
};

inline ConfNodePool::~ConfNodePool () {
  clear ();
}

inline void ConfNodePool::clear () {
  constexpr auto stride = sizeof (ConfNode);

  while (head_) {
    auto *chunk = head_.get ();
    auto *storage = reinterpret_cast<uint8_t *> (chunk->bytes);

    for (size_t offset = 0; offset + stride <= chunk->used; offset += stride) {
      reinterpret_cast<ConfNode *> (storage + offset)->~ConfNode ();
    }
    head_ = ystl::move (chunk->next);
  }
  count_ = 0;
}

inline ConfNode *ConfNodePool::create () {
  if (!head_ || head_->used + sizeof (ConfNode) > kChunkSize) {
    grow ();
  }
  auto *chunk = head_.get ();
  auto *node = new (reinterpret_cast<uint8_t *> (chunk->bytes) + chunk->used) ConfNode ();

  chunk->used += sizeof (ConfNode);
  node->pool_ = this;
  ++count_;

  return node;
}

inline void ConfNodePool::grow () {
  auto chunk = make_unique<Chunk> ();

  chunk->next = ystl::move (head_);
  head_ = ystl::move (chunk);
}

static_assert (alignof (ConfNode) <= alignof (void *), "ConfNodePool chunk storage is pointer-aligned");

// recursive-descent parser for the conf format
class ConfParser final : public NonCopyable {
public:
  enum : size_t {
    MaxDepth = 32,
    MaxIncludeDepth = 16
  };

private:
  Tokenizer scan_ {};
  String error_ {};
  ConfNode document_ {};

  ConfIncludeLoader include_loader_ = nullptr;
  void *include_user_ = nullptr;

public:
  ConfParser () = default;

public:
  // installs the loader used to expand @include lines (null disables includes)
  void set_include_loader (ConfIncludeLoader loader, void *user = nullptr) {
    include_loader_ = loader;
    include_user_ = user;
  }

  // parses the config text; on failure returns false (see error ()/errorline ()).
  // 'source' is the path of the text, used as the base for relative @include targets
  bool parse (StringRef text, StringRef source = {}) {
    error_.clear ();
    document_ = ConfNode {};

    if (include_loader_ != nullptr) {
      String expanded {};

      if (!expand_includes (text, source, 0, expanded)) {
        return false;
      }
      scan_ = Tokenizer { expanded };
      return parse_block (document_, 0);
    }
    scan_ = Tokenizer { text };
    return parse_block (document_, 0);
  }

  // parsed document root; valid until next parse ()
  const ConfNode &document () const {
    return document_;
  }

  // transfers document ownership out of the parser
  ConfNode take_document () {
    return ystl::move (document_);
  }

  StringRef error () const {
    return error_;
  }

  size_t error_line () const {
    return scan_.line ();
  }

private:
  // resolves a lone '@include "path"' / '@include path' line into the file text
  static bool ParseIncludeTarget (StringRef rest, String &out) {
    if (rest.empty ()) {
      return false;
    }
    if (rest[0] == '"') {
      const size_t end = rest.find ('"', 1);

      if (end == StringRef::InvalidIndex) {
        return false;
      }
      out.assign (rest.chars () + 1, end - 1);
      return !out.empty ();
    }
    size_t end = 0;

    while (end < rest.size () && !Tokenizer::is_space (rest[end]) && rest[end] != ';' && rest[end] != '#') {
      ++end;
    }
    if (end == 0) {
      return false;
    }
    out.assign (rest.chars (), end);
    return true;
  }

  // inlines @include lines recursively, the loader resolves a target against the file it came from
  bool expand_includes (StringRef text, StringRef from, size_t depth, String &out) {
    if (depth > MaxIncludeDepth) {
      error_.assign ("@include nesting is too deep");
      return false;
    }
    constexpr auto kDirective = "@include";
    constexpr size_t kDirectiveLen = sizeof ("@include") - 1;

    size_t pos = 0;

    for (;;) {
      const size_t end = text.find ('\n', pos);
      const size_t line_end = end == StringRef::InvalidIndex ? text.size () : end;
      const auto line = text.substr (pos, line_end - pos);
      const auto trimmed = Tokenizer::trim (line, " \t\r");

      bool is_include = trimmed.starts_with (kDirective);

      // the directive token must be followed by whitespace or a quote, not another word
      if (is_include && trimmed.size () > kDirectiveLen) {
        const auto after = trimmed[kDirectiveLen];
        is_include = after == ' ' || after == '\t' || after == '"';
      }
      if (is_include) {
        const auto rest = trimmed.size () > kDirectiveLen
                            ? Tokenizer::trim (trimmed.substr (kDirectiveLen, trimmed.size () - kDirectiveLen), " \t")
                            : StringRef {};
        String path {};

        if (!ParseIncludeTarget (rest, path)) {
          error_.assign ("malformed @include directive");
          return false;
        }

        String included = include_loader_ (path, from, include_user_);

        if (included.empty ()) {
          error_.assignf ("@include: cannot read '%s'", path.chars ());
          return false;
        }
        if (!expand_includes (included, path, depth + 1, out)) {
          return false;
        }
      }
      else {
        out.append (line.chars (), line.size ());
      }

      if (end == StringRef::InvalidIndex) {
        break;
      }
      out.append ('\n');
      pos = end + 1;
    }
    return true;
  }

  // creates a node in the document pool
  ConfNode *create_node () {
    return document_.ensure_pool ()->create ();
  }

  bool parse_block (ConfNode &parent, size_t depth) {
    if (depth > MaxDepth) {
      return fail (scan_.line (), "maximum nesting depth exceeded");
    }
    for (;;) {
      Array<String> comments {};
      skip_comments (comments);

      if (scan_.eof ()) {
        if (depth == 0) {
          return true;
        }
        return fail (scan_.line (), "unexpected end of file, missing '}'");
      }
      if (scan_.peek () == '}') {
        if (depth == 0) {
          return fail (scan_.line (), "unexpected '}'");
        }
        scan_.advance ();
        return true;
      }
      const auto stmt_line = scan_.line ();
      const auto stmt_start = scan_.pos ();
      auto *node = create_node ();

      node->comments_ = ystl::move (comments);

      if (!parse_name (node->name_)) {
        return fail (stmt_line, "expected key or section name");
      }
      scan_.skip_spaces ();

      // raw modifier declares a verbatim block with raw name or raw brace form
      if (node->name_ == "raw" && !scan_.eof () && scan_.peek () != '=' && (scan_.peek () == '{' || can_start_name (scan_.peek ()))) {
        if (scan_.peek () == '{') {
          node->name_.clear ();
        }
        else {
          String raw_name {};

          if (!parse_name (raw_name)) {
            return fail (stmt_line, "expected raw block name");
          }
          scan_.skip_spaces ();
          node->name_ = ystl::move (raw_name);
        }

        if (scan_.eof () || scan_.peek () != '{') {
          // looked like a raw block, but no '{' - fall back to a relaxed item
          node->name_.clear ();
          scan_.seek (stmt_start);

          if (!parse_value (node->value_, stmt_line)) {
            return false;
          }
          parent.children_.push (node);
          continue;
        }
        scan_.advance ();
        node->block_ = true;
        node->raw_ = true;

        if (!parse_raw_block (*node)) {
          return false;
        }
        parent.children_.push (node);
        continue;
      }

      // relaxed statement without equals or brace becomes an anonymous list item
      if (scan_.eof () || (scan_.peek () != '=' && scan_.peek () != '{')) {
        node->name_.clear ();
        scan_.seek (stmt_start);

        if (!parse_value (node->value_, stmt_line)) {
          return false;
        }
        parent.children_.push (node);
        continue;
      }
      if (scan_.accept ('{')) {
        node->block_ = true;

        if (!parse_block (*node, depth + 1)) {
          return false;
        }
      }
      else if (scan_.accept ('=')) {
        skip_whitespace_and_comments ();

        if (scan_.eof ()) {
          return fail (stmt_line, "expected value after '='");
        }
        if (scan_.accept ('{')) {
          node->block_ = true;

          if (!parse_block (*node, depth + 1)) {
            return false;
          }
        }
        else if (!parse_value (node->value_, stmt_line)) {
          return false;
        }
      }
      else {
        return fail (stmt_line, "expected '=' or '{' after key");
      }
      parent.children_.push (node);
    }
  }

  // raw block consumes lines verbatim until a line holding only closing brace
  bool parse_raw_block (ConfNode &parent) {
    for (;;) {
      scan_.skip_while (Tokenizer::is_space);

      if (scan_.eof ()) {
        return fail (scan_.line (), "unexpected end of file, missing '}' in raw block");
      }
      const auto line = scan_.read_until ('\n');
      scan_.accept ('\n'); // consume the line end when present

      const auto content = Tokenizer::trim (line, " \t\r");

      if (content.size () == 1 && content[0] == '}') {
        return true;
      }
      if (content.empty ()) {
        continue; // empty line
      }
      auto *node = create_node ();

      node->value_.assign (content.chars (), content.size ());
      parent.children_.push (node);
    }
  }

  // bare identifier (until delimiter) or quoted string
  bool parse_name (String &out) {
    if (scan_.eof ()) {
      return false;
    }
    if (scan_.peek () == '"') {
      return parse_quoted (out);
    }
    const auto name = scan_.read_while ([] (char ch) {
      return !is_delimiter (ch);
    });

    if (name.empty ()) {
      return false;
    }
    out.assign (name.chars (), name.size ());
    return true;
  }

  bool parse_quoted (String &out) {
    scan_.advance (); // skip opening quote
    out.clear ();

    while (!scan_.eof ()) {
      const auto ch = scan_.peek ();
      scan_.advance ();

      if (ch == '"') {
        return true;
      }
      if (ch == '\n') {
        return false; // unterminated string
      }
      if (ch == '\\' && !scan_.eof ()) {
        const auto esc = scan_.peek ();
        scan_.advance ();

        switch (esc) {
        case 'n':
          out += '\n';
          break;
        case 't':
          out += '\t';
          break;
        case '\\':
          out += '\\';
          break;
        case '"':
          out += '"';
          break;
        case '\n':
          out += '\n';
          break;
        default:
          out += esc;
          break;
        }
        continue;
      }
      out += ch;
    }
    return false; // unterminated string
  }

  // scalar value: quoted string, or the rest of the line (trimmed, inline comments stripped)
  bool parse_value (String &out, size_t stmt_line) {
    if (scan_.peek () == '"') {
      if (!parse_quoted (out)) {
        return fail (stmt_line, "unterminated quoted value");
      }
      scan_.skip_spaces ();

      if (!scan_.eof ()) {
        const auto ch = scan_.peek ();
        const auto next = scan_.peek (1);

        if (ch == ';' || ch == '#' || (ch == '/' && (next == '/' || next == '*'))) {
          scan_.skip_line ();
        }
      }
      return true;
    }
    auto span = scan_.read_while ([] (char ch) {
      return ch != '\n' && ch != '}';
    });

    // strip inline comment ("//", ";" or "#") preceded by whitespace
    for (size_t i = 0; i + 1 < span.size (); ++i) {
      const auto ch = span[i];

      if (ch != ' ' && ch != '\t' && ch != '\r') {
        continue;
      }
      const auto next = span[i + 1];

      if (next == ';' || next == '#' || (next == '/' && i + 2 < span.size () && span[i + 2] == '/')) {
        span = span.substr (0, i);
        break;
      }
    }
    span = Tokenizer::trim (span, " \t\r");

    if (span.empty ()) {
      return fail (stmt_line, "expected value after '='");
    }
    out.assign (span.chars (), span.size ());

    return true;
  }

private:
  // collects full-line comments preceding a statement into out
  void skip_comments (Array<String> &out) {
    for (;;) {
      scan_.skip_while (Tokenizer::is_space);

      if (scan_.eof ()) {
        return;
      }
      const auto ch = scan_.peek ();
      size_t prefix = 0;

      if (ch == ';' || ch == '#') {
        prefix = 1;
      }
      else if (ch == '/' && scan_.peek (1) == '/') {
        prefix = 2;
      }
      else if (ch == '/' && scan_.peek (1) == '*') {
        skip_block_comment ();
        continue;
      }
      if (prefix == 0) {
        return;
      }
      scan_.advance ();

      if (prefix == 2) {
        scan_.advance ();
      }
      // strip whitespace right after the comment prefix, but never cross the line end
      scan_.skip_while ([] (char c) {
        return c != '\n' && Tokenizer::is_space (c);
      });

      const auto comment = Tokenizer::trim (scan_.read_until ('\n'), " \t\r");
      scan_.accept ('\n'); // consume the line end when present

      out.push (comment);
    }
  }

  void skip_whitespace_and_comments () {
    for (;;) {
      scan_.skip_while (Tokenizer::is_space);

      if (scan_.eof ()) {
        return;
      }
      const auto ch = scan_.peek ();

      if (ch == ';' || ch == '#' || (ch == '/' && scan_.peek (1) == '/')) {
        scan_.skip_line ();
        continue;
      }
      if (ch == '/' && scan_.peek (1) == '*') {
        skip_block_comment ();
        continue;
      }
      return;
    }
  }

  void skip_block_comment () {
    scan_.advance ();
    scan_.advance ();

    while (!scan_.eof ()) {
      if (scan_.peek () == '*' && scan_.peek (1) == '/') {
        scan_.advance ();
        scan_.advance ();
        break;
      }
      scan_.advance (); // line tracking is handled by the tokenizer
    }
  }

  static constexpr bool is_delimiter (char ch) {
    switch (ch) {
    case ' ':
    case '\t':
    case '\r':
    case '\n':
    case '=':
    case '{':
    case '}':
    case ',':
    case ';':
    case '#':
      return true;
    }
    return false;
  }

  // true when a character can start a (quoted or bare) name
  static constexpr bool can_start_name (char ch) {
    return ch == '"' || (!Tokenizer::is_space (ch) && ch != '=' && ch != '}' && ch != '{');
  }

  bool fail (size_t line, StringRef message) {
    error_.assignf ("line %zu: %s", line, message.chars ());
    return false;
  }
};

// serializer for the conf format; mirrors the confparser grammar
class ConfWriter final : public NonCopyable {
public:
  // serializes node with all its children; usually called on the document root
  static String write (const ConfNode &node, size_t indent = 2) {
    ConfWriter writer (indent);

    writer.out_.reserve (estimate (node, 0, indent));
    writer.emit_children (node, 0);
    return ystl::move (writer.out_);
  }

private:
  String out_ {};
  size_t indent_ { 2 };

  explicit ConfWriter (size_t indent) : indent_ (indent) {}

  // estimates serialized size of the subtree for a single output reservation
  static size_t estimate (const ConfNode &node, size_t depth, size_t indent) {
    size_t total = 0;

    for (const auto &comment : node.comments ()) {
      total += depth * indent + comment.size () + 5;
    }
    if (node.is_block ()) {
      total += depth * indent + node.name ().size () + 10;

      for (const auto *child : node.children ()) {
        total += estimate (*child, depth + 1, indent);
      }
      return total + depth * indent + 2;
    }
    if (node.is_item ()) {
      return total + depth * indent + node.value ().size () + 8;
    }
    return total + depth * indent + node.name ().size () + node.value ().size () + 10;
  }

  void emit_children (const ConfNode &node, size_t depth) {
    for (const auto &child : node.children ()) {
      emit_statement (*child, depth);
    }
  }

  void emit_statement (const ConfNode &node, size_t depth) {
    for (const auto &comment : node.comments ()) {
      indent (depth);
      out_ += "// ";
      out_ += comment;
      out_ += '\n';
    }
    indent (depth);

    if (node.is_block ()) {
      if (node.name ().empty ()) {
        out_ += "raw {"; // anonymous blocks are only valid as raw
      }
      else if (node.is_raw ()) {
        out_ += "raw ";
        out_ += quoted (node.name ());
        out_ += " {";
      }
      else {
        out_ += quoted (node.name ());
        out_ += " {";
      }
      out_ += '\n';

      if (node.is_raw ()) {
        emit_raw_items (node, depth);
      }
      else {
        emit_children (node, depth + 1);
      }
      indent (depth);
      out_ += "}\n";
      return;
    }
    if (node.is_item ()) {
      out_ += quoted (node.value ());
      out_ += '\n';
      return;
    }
    out_ += quoted (node.name ());
    out_ += " = ";

    // empty scalars must be quoted, the parser has no bare empty values
    out_ += node.value ().empty () ? String ("\"\"") : quoted (node.value ());
    out_ += '\n';
  }

  void emit_raw_items (const ConfNode &node, size_t depth) {
    for (const auto &child : node.children ()) {
      // raw blocks are line-based, split multi-line items
      for (const auto &line : child->value ().split<String> ("\n")) {
        if (line.empty ()) {
          continue;
        }
        indent (depth + 1);
        out_ += line;
        out_ += '\n';
      }
    }
  }

  void indent (size_t depth) {
    for (size_t i = 0; i < depth; ++i) {
      for (size_t j = 0; j < indent_; ++j) {
        out_ += ' ';
      }
    }
  }

  // quotes name/value when it contains characters meaningful to the parser
  static String quoted (StringRef text) {
    if (!needs_quotes (text)) {
      return String (text);
    }
    String out {};

    out += '"';

    for (const auto &ch : text) {
      switch (ch) {
      case '"':
        out += "\\\"";
        break;
      case '\\':
        out += "\\\\";
        break;
      case '\n':
        out += "\\n";
        break;
      case '\t':
        out += "\\t";
        break;
      default:
        out += ch;
        break;
      }
    }
    out += '"';
    return out;
  }

  static bool needs_quotes (StringRef text) {
    if (text.empty () || text[0] == '/') {
      return true; // empty, or would start a line/inline comment
    }

    if (text == "raw") [[unlikely]] {
      return true; // block keyword, would parse as anonymous raw block
    }

    for (const auto &ch : text) {
      if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n' || ch == '=' || ch == '{' || ch == '}' || ch == ',' || ch == ';' || ch == '#' ||
          ch == '"') {
        return true;
      }
    }
    return false;
  }
};

}
