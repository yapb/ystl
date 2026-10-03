// SPDX-License-Identifier: Unlicense

#pragma once

#include <stdio.h>

#include <ystl/endian.h>
#include <ystl/span.h>
#include <ystl/string.h>
#include <ystl/lambda.h>
#include <ystl/twin.h>

namespace ystl {

// simple stdio file wrapper
class File final : private NonCopyable {
private:
  FILE *handle_ = nullptr;
  size_t size_ {};

public:
  explicit File () = default;

  File (StringRef file, StringRef mode = "rt") {
    open (file, mode);
  }

  ~File () {
    close ();
  }

public:
  bool open (StringRef file, StringRef mode) {
    close ();

    handle_ = plat.open_stdio_file (file.chars (), mode.chars ());

    if (!handle_) {
      return false;
    }
    fseek (handle_, 0L, SEEK_END);
    auto pos = ftell (handle_);

    size_ = pos > 0 ? static_cast<size_t> (pos) : 0;
    fseek (handle_, 0L, SEEK_SET);

    return true;
  }

  void close () {
    if (handle_) {
      fclose (handle_);
      handle_ = nullptr;
    }
    size_ = 0;
  }

  bool eof () const {
    return !handle_ || !!feof (handle_);
  }

  bool flush () const {
    return handle_ && fflush (handle_) == 0;
  }

  int get () const {
    return handle_ ? fgetc (handle_) : EOF;
  }

  bool get_line (String &line) {
    int ch = 0;

    // reuse a small array buffer so short lines need no heap allocation
    SmallArray<char> data {};

    line.clear ();

    while ((ch = get ()) != EOF) {
      data.push (static_cast<char> (ch));

      if (ch == '\n') {
        break;
      }
    }

    if (!data.empty ()) {
      line.assign (data.data (), data.size ());
    }
    return !line.empty ();
  }

  template <typename... Args> size_t puts (const char *fmt, Args &&...args) {
    if (!handle_) {
      return 0;
    }
    const auto *text = strings.format (fmt, ystl::forward<Args> (args)...);
    const auto len = strlen (text);

    return fputs (text, handle_) >= 0 ? len : 0;
  }

  bool puts (const char *buffer) {
    if (!handle_) {
      return false;
    }
    return fputs (buffer, handle_) >= 0;
  }

  int put_char (int ch) {
    return handle_ ? fputc (ch, handle_) : EOF;
  }

  size_t read (void *buffer, size_t size, size_t count = 1) {
    return handle_ ? fread (buffer, size, count, handle_) : 0;
  }

  size_t write (const void *buffer, size_t size, size_t count = 1) {
    return handle_ ? fwrite (buffer, size, count, handle_) : 0;
  }

  // span overloads; read returns the number of elements read
  template <typename U> size_t read (const ystl::Span<U> &data) {
    if (!handle_ || data.empty ()) {
      return 0;
    }
    return fread (data.data (), sizeof (U), data.size (), handle_);
  }

  template <typename U> size_t write (const ystl::Span<U> &data) {
    if (!handle_ || data.empty ()) {
      return 0;
    }
    return fwrite (data.data (), sizeof (U), data.size (), handle_);
  }

  // reads a structure from the on-disk little-endian layout into native form
  template <typename T> size_t le_read (T &value) {
    const auto result = read (&value, sizeof (T));
    leio::le_swap (value);

    return result;
  }

  // writes a structure from native form into the on-disk little-endian layout
  template <typename T> size_t le_write (const T &value) {
    T staging = value;
    leio::le_swap (staging);

    return write (&staging, sizeof (T));
  }

  bool seek (long offset, int origin) {
    if (!handle_) {
      return false;
    }
    if (origin != SEEK_SET && origin != SEEK_CUR && origin != SEEK_END) {
      return false;
    }
    return fseek (handle_, offset, origin) == 0;
  }

  void rewind () {
    if (handle_) {
      ::rewind (handle_);
    }
  }

  size_t size () const {
    return size_;
  }

  explicit operator bool () const {
    return handle_ != nullptr;
  }

public:
  static inline void make_path (const char *path) {
    String buffer (path);

    for (size_t i = 1; i < buffer.size (); ++i) {
      if (buffer.at (i) == *kPathSeparator) {
        buffer.at (i) = kNullChar;
        plat.create_directory (buffer.chars ());
        buffer.at (i) = *kPathSeparator;
      }
    }
    plat.create_directory (buffer.chars ());
  }
};

// storage backend for memory-mapped file loading
class FileLoader : public Singleton<FileLoader> {
public:
  using LoadFunction = Lambda<uint8_t *(const char *, int *)>;
  using FreeFunction = Lambda<void (void *)>;

private:
  LoadFunction load_fun_ {};
  FreeFunction free_fun_ {};

public:
  explicit FileLoader () = default;
  ~FileLoader () = default;

public:
  void initialize (LoadFunction loader, FreeFunction unloader) {
    load_fun_ = ystl::move (loader);
    free_fun_ = ystl::move (unloader);
  }

  uint8_t *load (StringRef file, int *size) {
    if (load_fun_) {
      return load_fun_ (file.chars (), size);
    }
    return nullptr;
  }

  void unload (void *buffer) {
    if (free_fun_) {
      free_fun_ (buffer);
    }
  }

public:
  static uint8_t *default_load (const char *path, int *size) {
    File file (path, "rb");

    if (!file) {
      *size = 0;
      return nullptr;
    }
    const auto len = file.size ();

    if (len > static_cast<size_t> (numeric_limits<int>::max ())) [[unlikely]] {
      plat.abort ("FileLoader::default_load() file exceeds int range");
    }
    *size = static_cast<int> (len);

    if (len == 0) [[unlikely]] {
      return mem::allocate<uint8_t> (1);
    }
    auto data = mem::allocate<uint8_t> (len);

    if (file.read (data, len) != 1) [[unlikely]] {
      mem::release (data);
      *size = 0;
      return nullptr;
    }
    return data;
  }

  static void default_unload (void *buffer) {
    mem::release (buffer);
  }

  static String load_to_string (StringRef filename) {
    int result = 0;
    auto buffer = default_load (filename.chars (), &result);

    if (result > 0 && buffer) {
      String data (reinterpret_cast<char *> (buffer), static_cast<size_t> (result));
      default_unload (buffer);

      return data;
    }
    return "";
  }
};

// in-memory read-only file
class MemFile final : public NonCopyable {
private:
  uint8_t *contents_ = nullptr;
  size_t size_ {};
  size_t pos_ {};

public:
  explicit MemFile () = default;

  MemFile (StringRef file) {
    open (file);
  }

  ~MemFile () {
    close ();
  }

public:
  bool open (StringRef file) {
    close ();

    int size = 0;
    contents_ = FileLoader::instance ().load (file.chars (), &size);
    size_ = size > 0 ? static_cast<size_t> (size) : 0;

    if (!contents_) {
      size_ = 0;
      return false;
    }
    return true;
  }

  void close () {
    if (contents_) {
      FileLoader::instance ().unload (contents_);
      contents_ = nullptr;
    }
    size_ = 0;
    pos_ = 0;
  }

  int get () {
    if (!contents_ || pos_ >= size_) {
      return EOF;
    }
    return static_cast<int> (contents_[pos_++]);
  }

  bool get_line (String &line) {
    int ch = 0;

    // same as file::getline: rely on sbo so short lines need no heap block
    SmallArray<char> data {};

    line.clear ();

    while ((ch = get ()) != EOF) {
      data.push (static_cast<char> (ch));

      if (ch == '\n') {
        break;
      }
    }

    if (!data.empty ()) {
      line.assign (data.data (), data.size ());
    }
    return !line.empty ();
  }

  size_t read (void *buffer, size_t size, size_t count = 1) {
    if (!contents_ || pos_ >= size_ || !buffer || !size || !count) {
      return 0;
    }
    auto remaining = size_ - pos_;
    auto requested = size * count;
    auto bytes = requested <= remaining ? requested : remaining;

    memcpy (buffer, &contents_[pos_], bytes);
    pos_ += bytes;

    return bytes / size;
  }

  // span overload; returns the number of elements read
  template <typename U> size_t read (const ystl::Span<U> &data) {
    if (!contents_ || pos_ >= size_ || data.empty ()) {
      return 0;
    }
    auto remaining = size_ - pos_;
    auto requested = data.length_bytes ();
    auto bytes = requested <= remaining ? requested : remaining;

    memcpy (data.data (), &contents_[pos_], bytes);
    pos_ += bytes;

    return bytes / sizeof (U);
  }

  // reads a structure from the on-disk little-endian layout into native form
  template <typename T> size_t le_read (T &value) {
    const auto result = read (&value, sizeof (T));
    leio::le_swap (value);

    return result;
  }

  bool seek (size_t offset, int origin) {
    if (!contents_) {
      return false;
    }

    switch (origin) {
    case SEEK_SET:
      if (offset > size_) [[unlikely]] {
        return false;
      }
      pos_ = offset;
      break;

    case SEEK_END:
      if (offset > size_) [[unlikely]] {
        return false;
      }
      pos_ = size_ - offset;
      break;

    case SEEK_CUR:
      if (offset > size_ - pos_) [[unlikely]] {
        return false;
      }
      pos_ += offset;
      break;

    default:
      return false;
    }
    return true;
  }

  void rewind () {
    pos_ = 0;
  }

  size_t size () const {
    return size_;
  }

  bool eof () const {
    return pos_ >= size_;
  }

  // direct access to the whole loaded buffer (e.g
  const uint8_t *data () const {
    return contents_;
  }

  StringRef view () const {
    if (!contents_ || !size_) {
      return {};
    }
    return { reinterpret_cast<const char *> (contents_), size_ };
  }

  explicit operator bool () const {
    return contents_ != nullptr && size_ > 0;
  }
};

namespace detail {
struct FileEnumeratorEntry : public NonCopyable {
  FileEnumeratorEntry () = default;
  ~FileEnumeratorEntry () = default;

#if defined(YSTL_WINDOWS)
  bool next {};
  String path {};
  HANDLE handle = INVALID_HANDLE_VALUE;
  WIN32_FIND_DATAA data {};
#else
  String path {};
  String mask {};
  DIR *dir {};
  dirent *entry {};
#endif
};
}

// file enumerator
class FileEnumerator : public NonCopyable {
private:
  UniquePtr<detail::FileEnumeratorEntry> entry_;

public:
  FileEnumerator (StringRef mask) : entry_ (ystl::make_unique<detail::FileEnumeratorEntry> ()) {
    start (mask);
  }

  ~FileEnumerator () {
    close ();
  }

public:
  void start (StringRef mask) {
    close ();

    auto sep = mask.find_last_of (kPathSeparator);

    if (sep != StringRef::InvalidIndex) {
      entry_->path = mask.substr (0, sep);
    }
    else {
      entry_->path = ".";
    }

#if defined(YSTL_WINDOWS)
    entry_->handle = FindFirstFileA (mask.chars (), &entry_->data);
    entry_->next = entry_->handle != INVALID_HANDLE_VALUE;
#else
    entry_->mask = sep != StringRef::InvalidIndex ? mask.substr (sep + 1) : mask;
    entry_->dir = opendir (entry_->path.chars ());

    if (entry_->dir) {
      next ();
    }
#endif
  }

  void close () {
#if defined(YSTL_WINDOWS)
    if (entry_->handle != INVALID_HANDLE_VALUE) {
      FindClose (entry_->handle);

      entry_->handle = INVALID_HANDLE_VALUE;
      entry_->next = false;
    }
#else
    if (entry_->dir) {
      closedir (entry_->dir);
      entry_->dir = nullptr;
    }
#endif
  }

  bool next () {
#if defined(YSTL_WINDOWS)
    entry_->next = !!FindNextFileA (entry_->handle, &entry_->data);
    return entry_->next;
#else
    if (!entry_->dir) [[unlikely]] {
      return false;
    }
    while ((entry_->entry = readdir (entry_->dir)) != nullptr) {
      if (!fnmatch (entry_->mask.chars (), entry_->entry->d_name, FNM_CASEFOLD | FNM_NOESCAPE | FNM_PERIOD)) {
        return true;
      }
    }
    return false;
#endif
  }

  String get_match () const {
    StringRef match {};

#if defined(YSTL_WINDOWS)
    if (!entry_->next) {
      return {};
    }
    match = entry_->data.cFileName;
#else
    if (!entry_->entry) [[unlikely]] {
      return {};
    }
    match = entry_->entry->d_name;
#endif
    return String::join ({ entry_->path, match }, kPathSeparator);
  }

  bool is_directory () const {
#if defined(YSTL_WINDOWS)
    if (!entry_->next) {
      return false;
    }
    return !!(entry_->data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
#else
    if (!entry_->entry) [[unlikely]] {
      return false;
    }
    if (entry_->entry->d_type == DT_DIR) {
      return true;
    }
    else if (entry_->entry->d_type == DT_UNKNOWN) {
      struct stat st {};

      if (stat (String::join ({ entry_->path, entry_->entry->d_name }, kPathSeparator).chars (), &st) != 0) {
        return false;
      }
      return S_ISDIR (st.st_mode);
    }
    return false;
#endif
  }

  operator bool () const {
#if defined(YSTL_WINDOWS)
    return entry_->next;
#else
    return entry_->dir != nullptr && entry_->entry != nullptr;
#endif
  }
};

}
