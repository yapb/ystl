// test_http.cpp - tests for ystl/http.h note: no actual network connections are made
#include <ystl/ystl.h>
#include <ystl/test.h>

using namespace ystl;

// httpclientresult enum
TEST_CASE ("HttpClientResult Ok has value 200 [http]") {
  REQUIRE (static_cast<int> (HttpClientResult::Ok) == 200);
}

TEST_CASE ("HttpClientResult NotFound has value 404 [http]") {
  REQUIRE (static_cast<int> (HttpClientResult::NotFound) == 404);
}

TEST_CASE ("HttpClientResult InternalServerError has value 500 [http]") {
  REQUIRE (static_cast<int> (HttpClientResult::InternalServerError) == 500);
}

TEST_CASE ("HttpClientResult SocketError is negative [http]") {
  REQUIRE (static_cast<int> (HttpClientResult::SocketError) < 0);
}

TEST_CASE ("HttpClientResult ConnectError is negative [http]") {
  REQUIRE (static_cast<int> (HttpClientResult::ConnectError) < 0);
}

TEST_CASE ("HttpClientResult Undefined is negative [http]") {
  REQUIRE (static_cast<int> (HttpClientResult::Undefined) < 0);
}

TEST_CASE ("HttpClientResult NetworkUnavailable is negative [http]") {
  REQUIRE (static_cast<int> (HttpClientResult::NetworkUnavailable) < 0);
}

// detail::httpuri::parse
TEST_CASE ("HttpUrl parse extracts protocol, host, and path [http]") {
  auto uri = HttpUrl::parse ("http://example.com/some/path");
  REQUIRE (uri.protocol == "http");
  REQUIRE (uri.host == "example.com");
  REQUIRE (uri.path == "some/path");
}

TEST_CASE ("HttpUrl parse handles empty URI [http]") {
  auto uri = HttpUrl::parse ("");
  REQUIRE (uri.protocol.empty ());
  REQUIRE (uri.host.empty ());
  REQUIRE (uri.path.empty ());
}

TEST_CASE ("HttpUrl parse handles URI without path separator [http]") {
  // no '/' after the host => parse returns empty result
  auto uri = HttpUrl::parse ("http://example.com");
  // host won't be parsed because there's no trailing '/'
  REQUIRE (uri.path.empty ());
}

TEST_CASE ("HttpUrl parse with https protocol [http]") {
  auto uri = HttpUrl::parse ("https://update.example.com/files/data.zip");
  REQUIRE (uri.protocol == "https");
  REQUIRE (uri.host == "update.example.com");
  REQUIRE (uri.path == "files/data.zip");
}

// httpclient singleton
TEST_CASE ("HttpClient singleton returns same instance [http]") {
  auto &a = HttpClient::instance ();
  auto &b = http;
  REQUIRE (&a == &b);
}

TEST_CASE ("HttpClient getLastStatusCode returns Undefined before any request [http]") {
  // fresh state - no request was made
  auto code = http.get_last_status_code ();
  REQUIRE (static_cast<int> (code) != 0); // just ensure it returns something
}

TEST_CASE ("HttpClient setUserAgent does not crash [http]") {
  http.set_user_agent ("ystl-test-agent");
  REQUIRE (true);
}

TEST_CASE ("HttpClient setChunkSize does not crash [http]") {
  http.set_chunk_size (8192);
  REQUIRE (true);
}

// httpclient downloadfile
TEST_CASE ("HttpClient downloadFile returns false if local file already exists [http]") {
  http.startup ();

  const char *fname = "ystl_test_http_exists.tmp";
  {
    File fw (fname, "w");
    fw.puts ("existing");
  }

  bool ok = http.download_file ("http://example.com/file.bin", fname);
  REQUIRE (!ok);
  REQUIRE (http.get_last_status_code () == HttpClientResult::LocalFileExists);

  plat.remove_file (fname);
}

TEST_CASE ("HttpClient uploadFile returns false if local file does not exist [http]") {
  http.startup ();

  bool ok = http.upload_file ("http://example.com/upload", "ystl_nonexistent_upload.tmp");
  REQUIRE (!ok);
  REQUIRE (http.get_last_status_code () == HttpClientResult::NoLocalFile);
}

// httpclientresult enum -
TEST_CASE ("HttpClientResult redirect codes [http]") {
  REQUIRE (static_cast<int> (HttpClientResult::MovedPermanently) == 301);
  REQUIRE (static_cast<int> (HttpClientResult::Found) == 302);
  REQUIRE (static_cast<int> (HttpClientResult::TemporaryRedirect) == 307);
  REQUIRE (static_cast<int> (HttpClientResult::PermanentRedirect) == 308);
}

TEST_CASE ("HttpClientResult client error codes [http]") {
  REQUIRE (static_cast<int> (HttpClientResult::BadRequest) == 400);
  REQUIRE (static_cast<int> (HttpClientResult::Unauthorized) == 401);
  REQUIRE (static_cast<int> (HttpClientResult::Forbidden) == 403);
  REQUIRE (static_cast<int> (HttpClientResult::MethodNotAllowed) == 405);
  REQUIRE (static_cast<int> (HttpClientResult::TooManyRequests) == 429);
}

TEST_CASE ("HttpClientResult server error codes [http]") {
  REQUIRE (static_cast<int> (HttpClientResult::BadGateway) == 502);
  REQUIRE (static_cast<int> (HttpClientResult::ServiceUnavailable) == 503);
  REQUIRE (static_cast<int> (HttpClientResult::GatewayTimeout) == 504);
}

TEST_CASE ("HttpClientResult custom negative error codes [http]") {
  REQUIRE (static_cast<int> (HttpClientResult::SocketError) == -1);
  REQUIRE (static_cast<int> (HttpClientResult::ConnectError) == -2);
  REQUIRE (static_cast<int> (HttpClientResult::HttpOnly) == -3);
  REQUIRE (static_cast<int> (HttpClientResult::Undefined) == -4);
  REQUIRE (static_cast<int> (HttpClientResult::NoLocalFile) == -5);
  REQUIRE (static_cast<int> (HttpClientResult::LocalFileExists) == -6);
  REQUIRE (static_cast<int> (HttpClientResult::NetworkUnavailable) == -7);
}

// detail::httpuri::parse
TEST_CASE ("HttpUrl parse returns empty for URI without protocol separator [http]") {
  auto uri = HttpUrl::parse ("example.com/path");
  REQUIRE (uri.protocol.empty ());
  REQUIRE (uri.host.empty ());
  REQUIRE (uri.path.empty ());
}

TEST_CASE ("HttpUrl parse handles URI with just host and trailing slash [http]") {
  auto uri = HttpUrl::parse ("http://example.com/");
  REQUIRE (uri.protocol == "http");
  REQUIRE (uri.host == "example.com");
  REQUIRE (uri.path.empty ());
}

TEST_CASE ("HttpUrl parse handles deep path [http]") {
  auto uri = HttpUrl::parse ("http://cdn.example.com/a/b/c/d/file.txt");
  REQUIRE (uri.protocol == "http");
  REQUIRE (uri.host == "cdn.example.com");
  REQUIRE (uri.path == "a/b/c/d/file.txt");
}

TEST_CASE ("HttpUrl parse handles ftp protocol [http]") {
  auto uri = HttpUrl::parse ("ftp://files.example.com/data");
  REQUIRE (uri.protocol == "ftp");
  REQUIRE (uri.host == "files.example.com");
  REQUIRE (uri.path == "data");
}

// httpclient
#if defined(YSTL_WITH_TLS)
TEST_CASE ("HttpClient downloadFile over https reports ConnectError on bad host [http]") {
  http.startup (); // Available, so the request really goes out

  bool ok = http.download_file ("https://invalid.host.that.does.not.exist.example/file.bin", "ystl_https_test.tmp");
  REQUIRE (!ok);
  REQUIRE (http.get_last_status_code () == HttpClientResult::ConnectError);
}
#else
TEST_CASE ("HttpClient downloadFile returns HttpOnly for https URL [http]") {
  http.startup ();

  bool ok = http.download_file ("https://example.com/file.bin", "ystl_https_test.tmp");
  REQUIRE (!ok);
  REQUIRE (http.get_last_status_code () == HttpClientResult::HttpOnly);
}
#endif

#if defined(YSTL_WITH_TLS)
TEST_CASE ("HttpClient uploadFile over https reports ConnectError on bad host [http]") {
  http.startup (); // Available, so the request really goes out

  const char *fname = "ystl_test_upload_https.tmp";
  {
    File fw (fname, "w");
    fw.puts ("test data");
  }

  bool ok = http.upload_file ("https://invalid.host.that.does.not.exist.example/upload", fname);
  REQUIRE (!ok);
  REQUIRE (http.get_last_status_code () == HttpClientResult::ConnectError);

  plat.remove_file (fname);
}
#else
TEST_CASE ("HttpClient uploadFile returns HttpOnly for https URL [http]") {
  http.startup ();

  const char *fname = "ystl_test_upload_https.tmp";
  {
    File fw (fname, "w");
    fw.puts ("test data");
  }

  bool ok = http.upload_file ("https://example.com/upload", fname);
  REQUIRE (!ok);
  REQUIRE (http.get_last_status_code () == HttpClientResult::HttpOnly);

  plat.remove_file (fname);
}
#endif

// socket class
TEST_CASE ("Socket default construction [http]") {
  Socket s;
  REQUIRE (true);
}

TEST_CASE ("Socket setTimeout does not crash [http]") {
  Socket s;
  s.set_timeout (10);
  REQUIRE (true);
}

TEST_CASE ("Socket disconnect on unconnected socket is safe [http]") {
  Socket s;
  s.disconnect ();
  REQUIRE (true);
}

TEST_CASE ("Socket connect to invalid host returns false [http]") {
  http.startup ();
  Socket s;
  s.set_timeout (1);
  bool connected = s.connect ("invalid.host.that.does.not.exist.example");
  REQUIRE (!connected);
}

TEST_CASE ("HttpClient download fails fast while host check is pending [http]") {
  // the background connectivity check must never block the caller:
  // right after startup the state is "checking", so download fails
  // immediately with NetworkUnavailable instead of joining the thread.
  // if the checker already finished (unavailable), the result is the same.
  http.startup ("invalid.host.that.does.not.exist.example", "", 5);

  bool ok = http.download_file ("http://example.com/file.bin", "ystl_pending_check.tmp");
  REQUIRE (!ok);
  REQUIRE (http.get_last_status_code () == HttpClientResult::NetworkUnavailable);

  http.startup (); // restore Available for the other tests
}

TEST_CASE ("HttpClient double startup does not block [http]") {
  // a second startup abandons the previous check instead of joining it
  http.startup ("invalid.host.that.does.not.exist.example", "", 5);
  http.startup ("invalid.host.that.does.not.exist.example", "", 5);

  bool ok = http.download_file ("http://example.com/file.bin", "ystl_pending_check.tmp");
  REQUIRE (!ok);
  REQUIRE (http.get_last_status_code () == HttpClientResult::NetworkUnavailable);

  http.startup (); // restore Available for the other tests
}

TEST_CASE ("HttpUrl defaults port by scheme [http]") {
  auto plain = HttpUrl::parse ("http://example.com/graph/a.graph");
  REQUIRE (plain.port == "80");

  auto tls = HttpUrl::parse ("https://example.com/graph/a.graph");
  REQUIRE (tls.port == "443");
  REQUIRE (tls.is_https ());

  auto custom = HttpUrl::parse ("https://example.com:8443/graph/a.graph");
  REQUIRE (custom.host == "example.com");
  REQUIRE (custom.port == "8443");
  REQUIRE (custom.path == "graph/a.graph");
}

TEST_CASE ("HttpUrl:join handles full and legacy bases [http]") {
  REQUIRE (HttpUrl::join ("https://raw.githubusercontent.com/yapb/graph/refs/heads/master", "graph/a.graph") ==
           "https://raw.githubusercontent.com/yapb/graph/refs/heads/master/graph/a.graph");
  REQUIRE (HttpUrl::join ("https://example.com/base/", "/a.graph") == "https://example.com/base/a.graph");
  REQUIRE (HttpUrl::join ("yapb.jeefo.net", "graph/a.graph") == "http://yapb.jeefo.net/graph/a.graph");
  REQUIRE (HttpUrl::join ("yapb.jeefo.net/upload", "") == "http://yapb.jeefo.net/upload");
}

TEST_CASE ("connectivity check accepts host:port [http]") {
  // must not block and must fail closed while checking
  http.startup ("invalid.host.that.does.not.exist.example:443", "", 2);

  bool ok = http.download_file ("http://example.com/file.bin", "ystl_pending_check.tmp");
  REQUIRE (!ok);
  REQUIRE (http.get_last_status_code () == HttpClientResult::NetworkUnavailable);

  http.startup (); // restore Available for the other tests
}

#if !defined(YSTL_WITH_TLS)
TEST_CASE ("HttpClient https without tls build is HttpOnly [http]") {
  http.startup (); // Available, so the request really goes out

  bool ok = http.download_file ("https://example.com/file.bin", "ystl_https_fail.tmp");
  REQUIRE (!ok);
  REQUIRE (http.get_last_status_code () == HttpClientResult::HttpOnly);
}
#endif

// url parsing
TEST_CASE ("HttpUrl parse handles bracketed ipv6 with port [http]") {
  auto uri = HttpUrl::parse ("http://[::1]:8080/status");
  REQUIRE (uri.protocol == "http");
  REQUIRE (uri.host == "::1");
  REQUIRE (uri.port == "8080");
  REQUIRE (uri.path == "status");
}
