// SPDX-License-Identifier: Unlicense

#pragma once

#include <stdio.h>

#include <ystl/string.h>
#include <ystl/files.h>
#include <ystl/logger.h>
#include <ystl/twin.h>
#include <ystl/atomic.h>
#include <ystl/platform.h>
#include <ystl/memory.h>
#include <ystl/uniqueptr.h>
#include <ystl/random.h>
#include <ystl/thread.h>

#if defined(YSTL_WINDOWS)
  #include <winsock2.h>
  #include <ws2tcpip.h>
#else
  #include <netinet/in.h>
  #include <sys/socket.h>
  #include <sys/types.h>
  #if !defined(YSTL_PSVITA)
    #include <sys/uio.h>
  #endif
  #include <arpa/inet.h>
  #include <unistd.h>
  #include <errno.h>
  #include <netdb.h>
  #include <fcntl.h>
#endif

// tls via vendored mbedtls (ext/mbedtls). enabled by YSTL_WITH_TLS from the
// build (cmake/vcxproj); without it https requests fail with HttpOnly.
#if defined(YSTL_WITH_TLS)
  #include <mbedtls/ssl.h>
  #include <mbedtls/error.h>
  #include <mbedtls/net_sockets.h>
  #include <mbedtls/x509_crt.h>
  #include <psa/crypto.h>
#endif

// status codes for http client
enum class HttpClientResult : int32_t {
  Continue = 100,
  SwitchingProtocol = 101,
  Processing = 102,
  EarlyHints = 103,

  Ok = 200,
  Created = 201,
  Accepted = 202,
  NonAuthoritativeInformation = 203,
  NoContent = 204,
  ResetContent = 205,
  PartialContent = 206,
  MultiStatus = 207,
  AlreadyReported = 208,
  ImUsed = 226,

  MultipleChoice = 300,
  MovedPermanently = 301,
  Found = 302,
  SeeOther = 303,
  NotModified = 304,
  UseProxy = 305,
  TemporaryRedirect = 307,
  PermanentRedirect = 308,

  BadRequest = 400,
  Unauthorized = 401,
  PaymentRequired = 402,
  Forbidden = 403,
  NotFound = 404,
  MethodNotAllowed = 405,
  NotAcceptable = 406,
  ProxyAuthenticationRequired = 407,
  RequestTimeout = 408,
  Conflict = 409,
  Gone = 410,
  LengthRequired = 411,
  PreconditionFailed = 412,
  PayloadTooLarge = 413,
  UriTooLong = 414,
  UnsupportedMediaType = 415,
  RangeNotSatisfiable = 416,
  ExpectationFailed = 417,
  ImaTeapot = 418,
  MisdirectedRequest = 421,
  UnprocessableEntity = 422,
  Locked = 423,
  FailedDependency = 424,
  TooEarly = 425,
  UpgradeRequired = 426,
  PreconditionRequired = 428,
  TooManyRequests = 429,
  RequestHeaderFieldsTooLarge = 431,
  UnavailableForLegalReasons = 451,

  InternalServerError = 500,
  NotImplemented = 501,
  BadGateway = 502,
  ServiceUnavailable = 503,
  GatewayTimeout = 504,
  HttpVersionNotSupported = 505,
  VariantAlsoNegotiates = 506,
  InsufficientStorage = 507,
  LoopDetected = 508,
  NotExtended = 510,
  NetworkAuthenticationRequired = 511,

  SocketError = -1,
  ConnectError = -2,
  HttpOnly = -3,
  Undefined = -4,
  NoLocalFile = -5,
  LocalFileExists = -6,
  NetworkUnavailable = -7
};

namespace ystl {

// simple http url parser
struct HttpUrl {
  String protocol {}, host {}, port {}, path {};

public:
  bool is_https () const {
    return protocol == "https";
  }

  static HttpUrl parse (StringRef uri) {
    HttpUrl result;

    if (uri.empty ()) {
      return result;
    }
    const size_t sep = uri.find ("://");

    if (sep == String::InvalidIndex) {
      return result;
    }
    result.protocol = uri.substr (0, sep);
    result.protocol.lowercase ();

    const size_t host_start = sep + 3;
    const size_t path_slash = uri.find ("/", host_start);

    String host_port;
    if (path_slash != String::InvalidIndex) {
      host_port = uri.substr (host_start, path_slash - host_start);
      result.path = uri.substr (path_slash + 1);
    }
    else {
      host_port = uri.substr (host_start);
    }

    size_t colon = String::InvalidIndex;

    if (!host_port.empty () && host_port[0] == '[') {
      const size_t bracket = host_port.find (']');

      if (bracket != String::InvalidIndex) {
        result.host = host_port.substr (1, bracket - 1);

        if (bracket + 1 < host_port.size () && host_port[bracket + 1] == ':') {
          result.port = host_port.substr (bracket + 2);
        }
      }
      else {
        result.host = host_port;
      }
    }
    else {
      colon = host_port.find (':');

      if (colon != String::InvalidIndex) {
        result.host = host_port.substr (0, colon);
        result.port = host_port.substr (colon + 1);
      }
      else {
        result.host = host_port;
      }
    }

    if (result.port.empty ()) {
      result.port = result.is_https () ? "443" : "80";
    }
    return result;
  }

  // joins a configured base with a relative path. base may be a full url
  static String join (StringRef base, StringRef rel) {
    String b { base };
    b.trim ();
    b.rtrim ("/");

    if (b.find ("://") == String::InvalidIndex) {
      b = String ("http://") + b;
    }

    String r { rel };
    r.trim ();
    r.ltrim ("/");

    if (r.empty ()) {
      return b;
    }
    return b + "/" + r;
  }
};

namespace detail {

struct HostPort {
  String host {}, port = "80";
};

// splits a plain "host[:port]" value (connectivity check, no url involved)
inline HostPort split_host_port (StringRef value) {
  HostPort result;
  String tmp { value };
  tmp.trim ();

  if (tmp.empty ()) {
    return result;
  }
  const size_t colon = tmp.find_last_of (":");

  if (colon != String::InvalidIndex) {
    String host = tmp.substr (0, colon);
    String port = tmp.substr (colon + 1);

    host.trim ();
    port.trim ();

    if (!host.empty () && !port.empty ()) {
      result.host = host;
      result.port = port;

      return result;
    }
  }
  result.host = tmp;

  return result;
}

struct SocketInit {
  static void start () {
#if defined(YSTL_WINDOWS)
    WSADATA wsa;

    if (WSAStartup (MAKEWORD (2, 2), &wsa) != 0) {
      logger.error ("Unable to initialize sockets.");
    }
#endif
  }

  static void stop () {
#if defined(YSTL_WINDOWS)
    WSACleanup ();
#endif
  }
};

// heap-shared dns job between the caller and the resolver thread
struct DnsJob {
  String host {};
  String port {};

  Atomic<int> state {}; // 0 = running, 1 = ready (owned by caller), 3 = abandoned (owned by resolver)
  int status = -1;
  addrinfo *result = nullptr;

  DnsJob () = default;
  DnsJob (StringRef h, StringRef p) : host (h), port (p) {}
};
}

// byte transport: plain tcp or tls. blocking with timeouts
class ITransport : public NonCopyable {
public:
  virtual ~ITransport () = default;

  virtual bool connect (StringRef hostname, StringRef port) = 0;
  virtual int32_t send (const void *buffer, int32_t length) = 0;
  virtual int32_t recv (void *buffer, int32_t length) = 0;
  virtual void disconnect () = 0;
  virtual void set_timeout (uint32_t timeout) = 0;
};

class Socket final : public ITransport {
private:
#if defined(YSTL_WINDOWS)
  using SocketType = SOCKET;
#else
  using SocketType = int32_t;
#endif

private:
#if defined(YSTL_WINDOWS)
  static constexpr SocketType kInvalidSocket = INVALID_SOCKET;
#else
  static constexpr SocketType kInvalidSocket = -1;
#endif

private:
  SocketType socket_;
  uint32_t timeout_;

public:
  Socket () : socket_ (kInvalidSocket), timeout_ (2) {}

  ~Socket () {
    disconnect ();
  }

public:
  // resolves hostname with a bounded wait. getaddrinfo itself blocks without
  // any timeout (long hang with dead dns / no internet)
  static bool resolve_with_timeout (StringRef hostname, StringRef port, addrinfo **out_addr, uint32_t timeout_ms) {
    if (out_addr) {
      *out_addr = nullptr;
    }

    if (hostname.empty ()) {
      return false;
    }
    auto *job = mem::allocate_and_construct<detail::DnsJob> (hostname, port);

    Thread resolver { [job] () {
      addrinfo hints {};
      ystl::memzero (&hints, sizeof (hints));

      constexpr auto kNumericServ = 0x00000008;

      hints.ai_flags = kNumericServ;
      hints.ai_family = AF_INET;
      hints.ai_socktype = SOCK_STREAM;

      addrinfo *res = nullptr;
      const int rc = getaddrinfo (job->host.chars (), job->port.chars (), &hints, &res);

      job->status = rc;
      job->result = (rc == 0) ? res : nullptr;

      int expected = 0;
      if (job->state.compare_exchange (expected, 1)) {
        return; // caller claims the result
      }

      // caller gave up, free everything here
      if (res) {
        freeaddrinfo (res);
      }
      mem::destruct_and_release (job);
    } };

    if (!resolver.ok ()) [[unlikely]] {
      mem::destruct_and_release (job);
      return false;
    }
    resolver.detach ();

    constexpr uint32_t kPollSliceMs = 10;

    uint32_t waited = 0;
    for (;;) {
      if (job->state.load () == 1) {
        addrinfo *res = job->result;
        const int rc = job->status;

        job->state.store (2);
        mem::destruct_and_release (job);

        if (rc != 0 || !res) {
          return false;
        }

        if (out_addr) {
          *out_addr = res;
        }
        else {
          freeaddrinfo (res);
        }
        return true;
      }

      if (waited >= timeout_ms) {
        int expected = 0;

        if (job->state.compare_exchange (expected, 3)) {
          return false; // resolver cleans up
        }
        continue; // finished concurrently, claim it above
      }
      ThisThread::sleep (kPollSliceMs);
      waited += kPollSliceMs;
    }
  }

public:
  bool connect (StringRef hostname, StringRef port = "80") override {
    // bound the dns part as well: getaddrinfo has no timeout of its own
    constexpr uint32_t kMaxDnsTimeoutMs = 3000;

    uint32_t dns_timeout_ms = timeout_ * 1000;

    if (dns_timeout_ms == 0 || dns_timeout_ms > kMaxDnsTimeoutMs) {
      dns_timeout_ms = kMaxDnsTimeoutMs;
    }
    addrinfo *result = nullptr;

    if (!resolve_with_timeout (hostname, port, &result, dns_timeout_ms)) {
      return false;
    }
    socket_ = socket (result->ai_family, result->ai_socktype, 0);

    if (socket_ == kInvalidSocket) {
      freeaddrinfo (result);
      return false;
    }
#if !defined(YSTL_WINDOWS)
    if (socket_ >= FD_SETSIZE) [[unlikely]] {
      disconnect ();
      freeaddrinfo (result);
      return false;
    }
#endif

    // set non-blocking mode for connect timeout
#if defined(YSTL_WINDOWS)
    u_long mode = 1;
    ioctlsocket (socket_, FIONBIO, &mode);
#else
    fcntl (socket_, F_SETFL, fcntl (socket_, F_GETFL, 0) | O_NONBLOCK);
#endif

    ::connect (socket_, result->ai_addr, static_cast<socklen_t> (result->ai_addrlen));

    fd_set write_set;
    FD_ZERO (&write_set);
#if defined(YSTL_WINDOWS)
    FD_SET (socket_, &write_set);

    timeval tv { static_cast<long> (timeout_), 0 };
    auto ret = select (0, nullptr, &write_set, nullptr, &tv);
#else
    FD_SET (socket_, &write_set);

    timeval tv { static_cast<time_t> (timeout_), 0 };
    auto ret = select (socket_ + 1, nullptr, &write_set, nullptr, &tv);
#endif

    if (ret <= 0) {
      disconnect ();
      freeaddrinfo (result);
      return false;
    }

    // check for connection error
    int err = 0;

    socklen_t len = sizeof (err);
    getsockopt (socket_, SOL_SOCKET, SO_ERROR, reinterpret_cast<char *> (&err), &len);

    if (err != 0) {
      disconnect ();
      freeaddrinfo (result);
      return false;
    }

    // restore blocking mode
#if defined(YSTL_WINDOWS)
    mode = 0;
    ioctlsocket (socket_, FIONBIO, &mode);
#else
    fcntl (socket_, F_SETFL, fcntl (socket_, F_GETFL, 0) & ~O_NONBLOCK);
#endif

    // set recv/send timeouts
#if defined(YSTL_WINDOWS)
    DWORD tv_timeout = timeout_ * 1000;
#else
    timeval tv_timeout { static_cast<time_t> (timeout_), 0 };
#endif

    setsockopt (socket_, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<char *> (&tv_timeout), static_cast<int32_t> (sizeof (tv_timeout)));
    setsockopt (socket_, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<char *> (&tv_timeout), static_cast<int32_t> (sizeof (tv_timeout)));

    freeaddrinfo (result);

    return true;
  }

  void set_timeout (uint32_t timeout) override {
    timeout_ = timeout;
  }

  void disconnect () override {
    if (socket_ == kInvalidSocket) {
      return;
    }
#if defined(YSTL_WINDOWS)
    closesocket (socket_);
#else
    close (socket_);
#endif
    socket_ = kInvalidSocket;
  }

public:
  int32_t send (const void *buffer, int32_t length) override {
    return static_cast<int32_t> (::send (socket_, static_cast<const char *> (buffer), static_cast<size_t> (length), 0));
  }

  int32_t recv (void *buffer, int32_t length) override {
    return static_cast<int32_t> (::recv (socket_, static_cast<char *> (buffer), static_cast<size_t> (length), 0));
  }

public:
  static int32_t YSTL_STDCALL sendto (
    int socket, const void *message, size_t length, int flags, const struct sockaddr *dest, int32_t dest_length) {

#if defined(YSTL_WINDOWS)
    WSABUF buffer = { static_cast<ULONG> (length), const_cast<char *> (reinterpret_cast<const char *> (message)) };
    DWORD send_length = 0;

    if (WSASendTo (socket, &buffer, 1, &send_length, flags, dest, dest_length, NULL, NULL) == SOCKET_ERROR) {
      errno = WSAGetLastError ();
      return -1;
    }
    return static_cast<int32_t> (send_length);
#else
    iovec iov = { const_cast<void *> (message), length };
    msghdr msg {};

    msg.msg_name = reinterpret_cast<void *> (const_cast<struct sockaddr *> (dest));
    msg.msg_namelen = static_cast<socklen_t> (dest_length);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    return static_cast<int32_t> (sendmsg (socket, &msg, flags));
#endif
  }
};

#if defined(YSTL_WITH_TLS)
class TlsSocket final : public ITransport {
private:
  Socket socket_ {};
  uint32_t timeout_ = 2;
  bool open_ = false;

  mbedtls_ssl_context ssl_ {};
  mbedtls_ssl_config conf_ {};
  mbedtls_x509_crt *ca_ = nullptr;
  bool configured_ = false;

public:
  TlsSocket () {
    mbedtls_ssl_init (&ssl_);
    mbedtls_ssl_config_init (&conf_);
  }

  ~TlsSocket () override {
    disconnect ();
  }

private:
  static int tls_send (void *ctx, const unsigned char *buf, size_t len) {
    auto *socket = static_cast<Socket *> (ctx);

    if (socket == nullptr || buf == nullptr || len == 0) {
      return MBEDTLS_ERR_NET_SEND_FAILED;
    }
    constexpr size_t kMaxChunk = 16384;

    const size_t chunk = len > kMaxChunk ? kMaxChunk : len;
    const int32_t sent = socket->send (buf, static_cast<int32_t> (chunk));

    if (sent <= 0) {
      return MBEDTLS_ERR_NET_SEND_FAILED;
    }
    return static_cast<int> (sent);
  }

  static int tls_recv (void *ctx, unsigned char *buf, size_t len) {
    auto *socket = static_cast<Socket *> (ctx);

    if (socket == nullptr || buf == nullptr || len == 0) {
      return MBEDTLS_ERR_NET_RECV_FAILED;
    }
    constexpr size_t kMaxChunk = 16384;

    const size_t chunk = len > kMaxChunk ? kMaxChunk : len;
    const int32_t got = socket->recv (buf, static_cast<int32_t> (chunk));

    if (got > 0) {
      return static_cast<int> (got);
    }

    if (got == 0) {
      return MBEDTLS_ERR_NET_CONN_RESET;
    }
    return MBEDTLS_ERR_NET_RECV_FAILED;
  }

public:
  void set_timeout (uint32_t timeout) override {
    timeout_ = timeout;
    socket_.set_timeout (timeout);
  }

  void set_ca (mbedtls_x509_crt *ca) {
    ca_ = ca;
  }

  bool connect (StringRef hostname, StringRef port = "443") override {
    disconnect ();

    if (hostname.empty ()) {
      return false;
    }
    socket_.set_timeout (timeout_);

    if (!socket_.connect (hostname, port)) {
      return false;
    }

    if (psa_crypto_init () != PSA_SUCCESS) {
      disconnect ();
      return false;
    }

    if (mbedtls_ssl_config_defaults (&conf_, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT) != 0) {
      disconnect ();
      return false;
    }
    if (ca_) {
      mbedtls_ssl_conf_authmode (&conf_, MBEDTLS_SSL_VERIFY_REQUIRED);
      mbedtls_ssl_conf_ca_chain (&conf_, ca_, nullptr);
    }
    else {
      mbedtls_ssl_conf_authmode (&conf_, MBEDTLS_SSL_VERIFY_NONE);
    }

    if (mbedtls_ssl_setup (&ssl_, &conf_) != 0) {
      disconnect ();
      return false;
    }
    configured_ = true;

    if (mbedtls_ssl_set_hostname (&ssl_, hostname.chars ()) != 0) {
      disconnect ();
      return false;
    }
    mbedtls_ssl_set_bio (&ssl_, &socket_, tls_send, tls_recv, nullptr);

    // every call is bounded by the socket timeouts, so this loop always terminates, with or without WANT retries
    constexpr int kMaxWantRetries = 128;

    int retries = 0;
    int ret = 0;

    while ((ret = mbedtls_ssl_handshake (&ssl_)) != 0) {
      if ((ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) || ++retries > kMaxWantRetries) {
        disconnect ();
        return false;
      }
    }
    open_ = true;

    return true;
  }

  int32_t send (const void *buffer, int32_t length) override {
    if (!open_ || buffer == nullptr || length <= 0) {
      return -1;
    }
    const auto *ptr = static_cast<const uint8_t *> (buffer);

    int32_t done = 0;
    int retries = 0;

    while (done < length) {
      const int ret = mbedtls_ssl_write (&ssl_, &ptr[done], static_cast<size_t> (length - done));

      if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
        if (++retries > 128) {
          return done > 0 ? done : -1;
        }
        continue;
      }

      if (ret <= 0) {
        return done > 0 ? done : -1;
      }
      done += ret;
    }
    return done;
  }

  int32_t recv (void *buffer, int32_t length) override {
    if (!open_ || buffer == nullptr || length <= 0) {
      return -1;
    }
    int retries = 0;
    int ret = 0;

    for (;;) {
      ret = mbedtls_ssl_read (&ssl_, static_cast<unsigned char *> (buffer), static_cast<size_t> (length));

      if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
        break;
      }

      if (++retries > 128) {
        return -1;
      }
    }

    if (ret > 0) {
      return static_cast<int32_t> (ret);
    }

    // orderly shutdown reads as eof, same as a closed tcp socket
    if (ret == 0 || ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY || ret == MBEDTLS_ERR_NET_CONN_RESET) {
      return 0;
    }
    return -1;
  }

  void disconnect () override {
    if (open_) {
      open_ = false;
      mbedtls_ssl_close_notify (&ssl_); // best effort on a live socket
    }

    if (configured_) {
      configured_ = false;

      mbedtls_ssl_free (&ssl_);
      mbedtls_ssl_config_free (&conf_);

      mbedtls_ssl_init (&ssl_);
      mbedtls_ssl_config_init (&conf_);
    }
    socket_.disconnect ();
  }
};
#endif // YSTL_WITH_TLS

struct HttpRequest {
  String method = "GET";
  String url {};
  String content_type {};
  String extra_headers {};
  size_t content_length = 0;
};

struct HttpResponse {
  HttpClientResult status_code = HttpClientResult::Undefined;
  int32_t content_length = -1;
  bool chunked = false;
  String location {};
};

// picks a transport by url scheme. returns null when the scheme has no transport
inline UniquePtr<ITransport> make_transport (const HttpUrl &url
#if defined(YSTL_WITH_TLS)
  ,
  mbedtls_x509_crt *ca = nullptr
#endif
) {
  if (url.is_https ()) {
#if defined(YSTL_WITH_TLS)
    auto socket = ystl::make_unique<TlsSocket> ();
    socket->set_ca (ca);
    return socket;
#else
    return UniquePtr<ITransport> {};
#endif
  }

  if (url.protocol.empty () || url.protocol == "http") {
    return ystl::make_unique<Socket> ();
  }
  return UniquePtr<ITransport> {};
}

// simple http client for downloading/uploading files only
class HttpClient final : public Singleton<HttpClient> {
private:
  enum : int32_t {
    MaxReceiveErrors = 12,
    DefaultSocketTimeout = 5
  };

private:
  String user_agent_ = "ystl";
  HttpClientResult status_code_ = HttpClientResult::Undefined;
  int32_t chunk_size_ = 4096;

  bool initialized_ {};

  // 0 = checking (fail-closed), 1 = available, 2 = unavailable.
  Atomic<int> conn_state_ {};
  Atomic<int> check_gen_ {};

  Thread host_check_thread_;

#if defined(YSTL_WITH_TLS)
  mbedtls_x509_crt ca_bundle_ {};
  bool ca_bundle_init_ = false;
  bool ca_loaded_ = false;
#endif

public:
  HttpClient () = default;

  ~HttpClient () {
#if defined(YSTL_WITH_TLS)
    if (ca_bundle_init_) {
      mbedtls_x509_crt_free (&ca_bundle_);
    }
#endif
    if (!initialized_) {
      return;
    }
    wait_ready ();
    detail::SocketInit::stop ();
  }

private:
  void wait_ready () {
    if (host_check_thread_.ok ()) {
      host_check_thread_.join ();
    }
  }

  bool check_connection () {
    return conn_state_.load () == 1;
  }

  static bool is_redirect (HttpClientResult code) {
    return code == HttpClientResult::MovedPermanently || code == HttpClientResult::Found || code == HttpClientResult::SeeOther ||
           code == HttpClientResult::TemporaryRedirect || code == HttpClientResult::PermanentRedirect;
  }

  HttpResponse parse_response_header (ITransport *transport) {
    HttpResponse info;
    Array<uint8_t> raw {};

    raw.reserve (static_cast<size_t> (chunk_size_));

    // accumulate until the header terminator, single-chunk responses overflow on fat headers
    constexpr size_t kMaxHeaderSize = 65536;
    int32_t errors = 0;

    for (;;) {
      if (raw.size () >= kMaxHeaderSize) {
        break;
      }
      uint8_t byte {};

      if (transport->recv (&byte, 1) < 1) {
        if (++errors > MaxReceiveErrors) {
          break;
        }
        continue;
      }
      raw.push (byte);

      const size_t count = raw.size ();

      if (count >= 4 && raw[count - 4] == '\r' && raw[count - 3] == '\n' && raw[count - 2] == '\r' && raw[count - 1] == '\n') {
        break;
      }
    }
    String response { reinterpret_cast<const char *> (raw.data ()), raw.size () };
    const size_t response_code_start = response.find ("HTTP/1.");

    if (response_code_start != String::InvalidIndex) {
      const size_t code_space = response.find (' ', response_code_start);

      if (code_space != String::InvalidIndex) {
        String resp_code = response.substr (code_space + 1, 3);

        resp_code.trim ();
        info.status_code = static_cast<HttpClientResult> (resp_code.as<int> ());
      }
    }

    // parse headers for content-length and location
    const size_t header_end = response.find ("\r\n\r\n");

    if (header_end != String::InvalidIndex) {
      size_t line_start = response.find ("\r\n", response_code_start);

      while (line_start != String::InvalidIndex && line_start < header_end) {
        line_start += 2;
        const size_t line_end = response.find ("\r\n", line_start);

        if (line_end == String::InvalidIndex || line_end > header_end) {
          break;
        }
        const size_t colon_pos = response.find (':', line_start);

        if (colon_pos != String::InvalidIndex && colon_pos < line_end) {
          String name = response.substr (line_start, colon_pos - line_start);
          String value = response.substr (colon_pos + 1, line_end - colon_pos - 1);

          value.trim ();

          if (name.equals_no_case ("Content-Length")) {
            info.content_length = value.as<int> ();
          }
          else if (name.equals_no_case ("Location")) {
            info.location = value;
          }
          else if (name.equals_no_case ("Transfer-Encoding")) {
            String encoding = value;
            encoding.lowercase ();

            info.chunked = encoding.contains ("chunked");
          }
        }
        line_start = line_end;
      }
    }
    return info;
  }

  // decodes chunked transfer encoding into file, framing never lands on disk
  bool recv_chunked_body (ITransport *transport, File &file) {
    SmallArray<uint8_t> piece (static_cast<size_t> (chunk_size_));
    int32_t errors = 0;
    String line {};

    // single text line reader for chunk sizes and trailers
    auto read_line = [&] () -> bool {
      line.clear ();

      for (;;) {
        uint8_t byte {};

        if (transport->recv (&byte, 1) < 1) {
          if (++errors > MaxReceiveErrors) {
            return false;
          }
          continue;
        }

        if (byte == '\n') {
          return true;
        }

        if (byte != '\r' && line.size () < 256) {
          line += static_cast<char> (byte);
        }
      }
    };

    for (;;) {
      if (!read_line () || line.empty ()) {
        return false; // truncated stream, not a terminator
      }
      const size_t semi = line.find (';');

      if (semi != String::InvalidIndex) {
        line = line.substr (0, semi); // chunk extensions ignored
      }
      line.trim ();

      // hex chunk size
      int64_t chunk = 0;
      bool any_digit = false;

      for (size_t k = 0; k < line.size (); ++k) {
        const char ch = line[k];
        int digit = -1;

        if (ch >= '0' && ch <= '9') {
          digit = ch - '0';
        }
        else if (ch >= 'a' && ch <= 'f') {
          digit = ch - 'a' + 10;
        }
        else if (ch >= 'A' && ch <= 'F') {
          digit = ch - 'A' + 10;
        }
        else {
          break;
        }
        any_digit = true;

        if (chunk > (numeric_limits<int64_t>::max () - digit) / 16) [[unlikely]] {
          return false; // chunk size overflows, malformed framing
        }
        chunk = chunk * 16 + digit;
      }

      if (!any_digit) {
        return false; // malformed framing, not a terminator
      }

      if (chunk == 0) {
        // zero chunk ends the body, trailers best-effort
        for (int t = 0; t < 32 && read_line (); ++t) {
          if (line.empty ()) {
            break;
          }
        }
        return true;
      }
      int64_t left = chunk;

      // exact byte count into file
      while (left > 0) {
        const int32_t want = left < chunk_size_ ? static_cast<int32_t> (left) : chunk_size_;
        const int32_t got = transport->recv (piece.data (), want);

        if (got < 1) {
          if (++errors > MaxReceiveErrors) {
            return false;
          }
          continue;
        }
        file.write (piece.data (), static_cast<size_t> (got));
        left -= got;
      }

      // trailing CRLF after payload
      for (int t = 0; t < 8; ++t) {
        uint8_t byte {};

        if (transport->recv (&byte, 1) < 1) {
          if (++errors > MaxReceiveErrors) {
            return false;
          }
          continue;
        }

        if (byte == '\n') {
          break;
        }

        if (t == 7) {
          return false;
        }
      }
    }
  }

  String build_request (const HttpRequest &req, const HttpUrl &url) {
    String request {};
    request.appendf ("%s /%s HTTP/1.1\r\n", req.method.chars (), url.path.chars ());

    const bool default_port = (!url.is_https () && url.port == "80") || (url.is_https () && url.port == "443");

    if (default_port) {
      request.appendf ("Host: %s\r\n", url.host.chars ());
    }
    else {
      request.appendf ("Host: %s:%s\r\n", url.host.chars (), url.port.chars ());
    }
    request.appendf ("User-Agent: %s\r\n", user_agent_.chars ());
    request.append ("Accept: */*\r\n");
    request.append ("Connection: close\r\n");

    if (req.content_length > 0) {
      request.appendf ("Content-Length: %zu\r\n", req.content_length);
    }
    if (!req.content_type.empty ()) {
      request.appendf ("Content-Type: %s\r\n", req.content_type.chars ());
    }
    if (!req.extra_headers.empty ()) {
      request.append (req.extra_headers.chars ());
    }
    request.append ("\r\n");

    return request;
  }

  // opens a transport for the url (scheme dispatch) and connects it
  bool open_transport (const HttpUrl &url, UniquePtr<ITransport> &transport, int32_t timeout, HttpClientResult &status) {
    (void)status;
#if defined(YSTL_WITH_TLS)
    transport = make_transport (url, ca_loaded_ ? &ca_bundle_ : nullptr);
#else
    transport = make_transport (url);
#endif

    if (!transport) {
      status = HttpClientResult::HttpOnly;
      return false;
    }
    transport->set_timeout (static_cast<uint32_t> (timeout));

    if (!transport->connect (url.host, url.port)) {
      status = HttpClientResult::ConnectError;
      return false;
    }
    status = HttpClientResult::Ok;

    return true;
  }

  // sends request headers over a connected transport
  bool send_head (ITransport *transport, const HttpRequest &req, const HttpUrl &url) {
    const String head = build_request (req, url);

    return transport->send (head.chars (), static_cast<int32_t> (head.size ())) > 0;
  }

public:
  void startup (StringRef host_check = "", StringRef err_message_if_host_down = "", uint32_t timeout = DefaultSocketTimeout) {
    detail::SocketInit::start ();

    initialized_ = true;

    if (host_check.empty ()) {
      check_gen_.fetch_add (1);
      conn_state_.store (1);
      return;
    }

    // abandon the previous check without blocking, Thread::start/assign would join()
    if (host_check_thread_.ok ()) {
      host_check_thread_.detach ();
    }
    conn_state_.store (0);

    const auto gen = check_gen_.fetch_add (1) + 1;
    const auto check = detail::split_host_port (host_check);

    String host_copy { check.host };
    String port_copy { check.port };
    String err_copy { err_message_if_host_down };

    host_check_thread_ = Thread { [this, host_copy, port_copy, err_copy, timeout, gen] () {
      Socket socket {};
      socket.set_timeout (timeout);

      const bool connected = socket.connect (host_copy, port_copy);

      if (gen != check_gen_.load ()) {
        return; // superseded by a newer startup(), don't overwrite its state
      }
      conn_state_.store (connected ? 1 : 2);

      if (!connected && !err_copy.empty ()) {
        logger.message (err_copy.chars ());
      }
    } };
  }

  bool download_file (StringRef url, StringRef local_path, int32_t timeout = DefaultSocketTimeout) {
    if (plat.win && !initialized_) {
      plat.abort ("Sockets not initialized.");
    }

    if (!check_connection ()) {
      status_code_ = HttpClientResult::NetworkUnavailable;
      return false;
    }

    if (plat.file_exists (local_path.chars ())) {
      status_code_ = HttpClientResult::LocalFileExists;
      return false;
    }

    constexpr int32_t kMaxRedirects = 5;
    String current_url = url;

    for (int32_t redirect_count = 0; redirect_count <= kMaxRedirects; ++redirect_count) {
      const auto uri = HttpUrl::parse (current_url);

      UniquePtr<ITransport> transport;
      HttpClientResult open_status = HttpClientResult::Undefined;

      if (!open_transport (uri, transport, timeout, open_status)) {
        status_code_ = open_status;
        return false;
      }
      HttpRequest req {};

      req.method = "GET";
      req.url = current_url;

      if (!send_head (transport.get (), req, uri)) {
        status_code_ = HttpClientResult::SocketError;
        return false;
      }
      SmallArray<uint8_t> buffer (static_cast<size_t> (chunk_size_));

      const auto response = parse_response_header (transport.get ());
      status_code_ = response.status_code;

      if (status_code_ == HttpClientResult::Ok) {
        File file (local_path, "wb");

        if (!file) {
          status_code_ = HttpClientResult::Undefined;
          return false;
        }

        // chunked bodies carry framing, decode before writing
        if (response.chunked) {
          return recv_chunked_body (transport.get (), file);
        }
        int32_t length = 0;
        int32_t errors = 0;
        int64_t total_received = 0;

        for (;;) {
          length = transport->recv (buffer.data (), chunk_size_);

          if (length > 0) {
            file.write (buffer.data (), static_cast<size_t> (length));
            total_received += length;

            if (response.content_length > 0 && total_received >= response.content_length) {
              break;
            }
          }
          else if (length == 0) {
            break;
          }
          else if (++errors > MaxReceiveErrors) {
            return false;
          }
        }
        return true;
      }

      if (!is_redirect (status_code_) || response.location.empty ()) {
        return false;
      }

      // resolve relative redirect url (may switch schemes, e.g. http -> https)
      if (response.location.find ("://") == String::InvalidIndex) {
        const size_t last_slash = current_url.find_last_of ("/");

        if (last_slash != String::InvalidIndex && last_slash > current_url.find ("://") + 2) {
          current_url = current_url.substr (0, last_slash + 1) + response.location;
        }
        else {
          current_url = current_url + "/" + response.location;
        }
      }
      else {
        current_url = response.location;
      }
    }
    status_code_ = HttpClientResult::Undefined;
    return false;
  }

  bool upload_file (StringRef url, StringRef local_path, const int32_t timeout = DefaultSocketTimeout) {
    if (plat.win && !initialized_) {
      plat.abort ("Sockets not initialized.");
    }

    if (!check_connection ()) {
      status_code_ = HttpClientResult::NetworkUnavailable;
      return false;
    }

    if (!plat.file_exists (local_path.chars ())) {
      status_code_ = HttpClientResult::NoLocalFile;
      return false;
    }
    const auto uri = HttpUrl::parse (url);

    UniquePtr<ITransport> transport;
    HttpClientResult open_status = HttpClientResult::Undefined;

    if (!open_transport (uri, transport, timeout, open_status)) {
      status_code_ = open_status;
      return false;
    }
    File file (local_path, "rb");

    if (!file) {
      status_code_ = HttpClientResult::Undefined;
      return false;
    }
    String boundary_name = local_path;
    const size_t boundary_slash = local_path.find_last_of ("\\/");

    if (boundary_slash != String::InvalidIndex) {
      boundary_name = local_path.substr (boundary_slash + 1);
    }
    StringRef boundary_line = strings.format ("---ystl_upload_boundary_%d%d%d%d", rg (0, 9), rg (0, 9), rg (0, 9), rg (0, 9));

    // sanitize filename for content-disposition
    String safe_name {};

    for (size_t i = 0; i < boundary_name.size (); ++i) {
      const char c = boundary_name[i];

      if (c == '"' || c == '\\' || c == '\r' || c == '\n') {
        safe_name += '_';
      }
      else {
        safe_name += c;
      }
    }
    String start {}, end {};

    start.appendf ("--%s\r\n", boundary_line);
    start.appendf ("Content-Disposition: form-data; name=\"file\"; filename=\"%s\"\r\n", safe_name.chars ());
    start.append ("Content-Type: application/octet-stream\r\n\r\n");

    end.appendf ("\r\n--%s--\r\n\r\n", boundary_line);

    const auto content_length = static_cast<size_t> (file.size ()) + start.size () + end.size ();
    StringRef content_type = strings.format ("multipart/form-data; boundary=%s", boundary_line);

    HttpRequest req;

    req.method = "POST";
    req.url = url;
    req.content_type = content_type;
    req.content_length = content_length;

    if (!send_head (transport.get (), req, uri)) {
      status_code_ = HttpClientResult::SocketError;
      return false;
    }

    if (transport->send (start.chars (), static_cast<int32_t> (start.size ())) < 1) {
      status_code_ = HttpClientResult::SocketError;
      return false;
    }
    SmallArray<uint8_t> buffer (static_cast<size_t> (chunk_size_));
    int32_t length = 0;

    for (;;) {
      length = static_cast<int32_t> (file.read (buffer.data (), 1, static_cast<size_t> (chunk_size_)));

      if (length > 0) {
        transport->send (buffer.data (), length);
      }
      else {
        break;
      }
    }

    if (transport->send (end.chars (), static_cast<int32_t> (end.size ())) < 1) {
      status_code_ = HttpClientResult::SocketError;
      return false;
    }
    status_code_ = parse_response_header (transport.get ()).status_code;

    return status_code_ == HttpClientResult::Ok;
  }

public:
  static constexpr bool has_tls_support () {
#if defined(YSTL_WITH_TLS)
    return true;
#else
    return false;
#endif
  }

  void set_user_agent (StringRef ua) {
    user_agent_ = ua;
  }

  // loads a pem ca bundle for tls verification, true when usable.
  // without it https falls back to unverified connections.
  [[nodiscard]] bool set_ca_file ([[maybe_unused]] StringRef path) {
#if defined(YSTL_WITH_TLS)
    if (path.empty ()) [[unlikely]] {
      return false;
    }

    if (!ca_bundle_init_) {
      mbedtls_x509_crt_init (&ca_bundle_);
      ca_bundle_init_ = true;
    }

    if (psa_crypto_init () != PSA_SUCCESS) {
      return false;
    }
    File file (path.chars (), "rb");

    if (!file) {
      return false;
    }
    const auto len = file.size ();

    if (len == 0 || len > 4 * 1024 * 1024) [[unlikely]] {
      return false;
    }
    Array<uint8_t> der (len + 1);

    if (file.read (der.data (), len) != 1) [[unlikely]] {
      return false;
    }
    der[len] = 0;

    // positive means some certs failed but the rest loaded, only negative is fatal
    ca_loaded_ = mbedtls_x509_crt_parse (&ca_bundle_, der.data (), len + 1) >= 0;
    return ca_loaded_;
#else
    return false;
#endif
  }

  HttpClientResult get_last_status_code () {
    return status_code_;
  }

  void set_chunk_size (int32_t chunk_size) {
    if (chunk_size > 0) [[likely]] {
      chunk_size_ = chunk_size;
    }
  }
};

// expose global http client
YSTL_EXPOSE_GLOBAL_SINGLETON (HttpClient, http);

}
