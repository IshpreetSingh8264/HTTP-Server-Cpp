# HTTP Server (C++23) — Architecture

CodeCrafters "Build your own HTTP server". 14 stages. No frameworks: raw BSD
sockets, `std::thread`, and zlib.

- **Entry point:** `src/main.cpp` (72 lines)
- **Build:** CMake, `target_include_directories(... PRIVATE src)`, so headers are
  reached as `"<layer>/<Module>.hpp"`
- **Port:** 4221
- **Run:** `./your_program.sh --directory /tmp/files -v`

---

## 1. Overview

The server is a four-stage pipeline. Each stage knows only about the one below
it; nothing calls back up.

```
  socket accept
       │
       ▼
  server::Server ──────────── owns the listening socket, the thread pool and
       │                       the component graph
       ▼
  server::ThreadPool ──────── one task per accepted connection
       │
       ▼
  handlers::ConnectionHandler  owns one socket for its lifetime: reads exactly
       │                       one request at a time, keeps the rest buffered
       ▼                        (this is what makes pipelining work)
  http::HttpRequest           typed value object, header lookup is
       │                       case-insensitive
       ▼
  handlers::RouteHandler ──── thin dispatcher, holds the FileHandler
       │
       ▼
  handlers::routes            THE ROUTING TABLE (data, not control flow)
       │
       ├─► handlers::FileHandler     sandboxed filesystem access
       └─► compression::EncodingNegotiator → ResponseEncoder
```

`main.cpp` wires it together and contains no domain logic.

## 2. Key components

| Component | File | Responsibility |
|---|---|---|
| `server::Server` | `src/server/Server.{hpp,cpp}` | socket/bind/listen/accept; owns every other component |
| `server::ThreadPool` | `src/server/ThreadPool.{hpp,cpp}` | fixed worker pool, `hardware_concurrency()` threads |
| `handlers::ConnectionHandler` | `src/handlers/ConnectionHandler.{hpp,cpp}` | request framing, keep-alive loop, 30 s timeout, 1 MB cap |
| `http::HttpRequest` | `src/http/HttpRequest.{hpp,cpp}` | parse a raw request; repeated headers are comma-joined |
| `http::HttpResponse` | `src/http/HttpResponse.{hpp,cpp}` | build a response; keeps `Content-Length` in sync |
| `http::HttpConstants` | `src/http/HttpConstants.{hpp,cpp}` | status codes, methods, headers, MIME map |
| `handlers::RouteHandler` | `src/handlers/RouteHandler.{hpp,cpp}` | **dispatcher only** — 19 lines |
| `handlers::routes::*` | `src/handlers/routes/` | the route table and one file per endpoint |
| `handlers::FileHandler` | `src/handlers/FileHandler.{hpp,cpp}` | read/write files, path-traversal sandbox |
| `compression::EncodingNegotiator` | `src/compression/encoding_negotiator.{hpp,cpp}` | `Accept-Encoding` → one `Content-Encoding` |
| `compression::ResponseEncoder` | `src/compression/response_encoder.{hpp,cpp}` | gzip (RFC 1952) and raw deflate (RFC 1951) |
| `utils::StringUtils` | `src/utils/StringUtils.{hpp,cpp}` | pure string helpers, no domain knowledge |
| `utils::Logger` | `src/utils/Logger.{hpp,cpp}` | level-filtered, mutex-guarded, stderr only |

## 3. Data flow

### One GET

```
curl → recv() → readBuffer_ → readRequest() extracts exactly one request
     → HttpRequest::parse → RouteHandler::handleRequest
     → routes::dispatch → pathMatches() finds the row
     → row.handler(RouteContext) builds an HttpResponse
     → EncodingNegotiator picks a coding, ResponseEncoder compresses
     → HttpResponse::toString() → send() → curl
```

### Routing is a table

`src/handlers/routes/route_registry.cpp` holds the only routing mechanism:

```cpp
const std::vector<Route>& table() {
    static const std::vector<Route> kRouteTable = {
        {METHOD_GET,  "/",                &handleRoot},
        {METHOD_GET,  "/echo/{str}",      &handleEcho},
        {METHOD_GET,  "/user-agent",      &handleUserAgent},
        {METHOD_GET,  "/files/{name...}", &handleFileGet},
        {METHOD_POST, "/files/{name...}", &handleFilePost},
        {METHOD_HEAD, "/",                asHead(&handleRoot)},
        // ... one row per HEAD-wrapped GET
    };
    return kRouteTable;
}
```

There is no `if` on method and no `if` on path. Every row has the same
signature (`RouteHandlerFn`), and every handler receives one typed
`RouteContext` rather than reaching for the dispatcher.

Path patterns: `/files/{name}` matches one segment, `/files/{name...}` is a
greedy tail that matches the rest of the path. Parameters are positional; a
handler that wants one re-derives it from `request.getPath()`.

Resolution: method+path → call it · path only → **405** · nothing → **404**.

### Compression negotiation

`Accept-Encoding` is parsed into `(coding, q, position)`, then:

1. highest `q` wins; `coding;q=0` rejects that coding
2. on a `q` tie, the coding the client listed **first** wins
3. a bare `*` accepts any coding we can produce
4. `identity` is the fallback, so an absent or unusable header gives an
   uncompressed response rather than a 406

`gzip` is `deflateInit2` with `windowBits = 15 + 16`; `deflate` is
`windowBits = -15`, a raw RFC 1951 stream, which is what browsers and curl
actually exchange.

Compression lives in the **dispatcher**, not in the handlers: whether a body
may be compressed depends on the request and the response, not on the endpoint,
so no handler has to remember to ask.

## 4. Conventions

- **Namespaces are the directories, one-to-one.** `http`, `server`, `handlers`,
  `compression`, `utils`. No new top-level namespace.
- **Every header has a `.cpp`.** Declarations in the header, definitions in the
  translation unit. No header-only classes, no inline bodies in headers.
- **Includes are `"<layer>/<Module>.hpp"`.** `src` is the include root.
- **Every class is a set of real declarations, not an inline blob.** In-class
  definitions are reserved for one-liners (getters, trivial setters).
- **One concept per file.** The largest file in the project is 194 lines; the
  limit is 600.
- **`main.cpp` holds no domain logic.** If you find yourself adding an endpoint
  there, stop.
- **Logs go to stderr only, at `ERROR` by default.** A test harness reads the
  socket, not our stderr; at `INFO` a single request emitted a dozen lines.
  `--verbose` raises it to `DEBUG`.
- **Comments explain *why*, not *what*.** Bilingual Pinglish comments are
  welcome and intentional; they are not a substitute for stating the invariant.
- **No `using namespace` in headers**, and no `std` in a signature.
- **Every non-obvious behaviour is proven by a test**, not by a comment.

## 5. Module map

```
src/
  main.cpp                          thin: parse args, install handler, start
  http/
    HttpConstants.{hpp,cpp}         status/method/header/MIME vocabulary
    HttpRequest.{hpp,cpp}           parse a raw request
    HttpResponse.{hpp,cpp}          build and serialise a response
  handlers/
    ConnectionHandler.{hpp,cpp}     socket lifecycle, framing, keep-alive
    FileHandler.{hpp,cpp}           sandboxed file IO
    RouteHandler.{hpp,cpp}          the dispatcher (delegates to routes/)
    routes/
      route_registry.{hpp,cpp}      THE TABLE + matcher + dispatcher
      route_handlers.hpp            one declaration per endpoint
      root_route.cpp                GET /
      echo_route.cpp                GET /echo/{str}
      user_agent_route.cpp          GET /user-agent
      files_route.cpp               GET|POST /files/{name}
      head_adapter.{hpp,cpp}        HEAD = GET minus the body
  compression/
    content_encoding.{hpp,cpp}      the codings we can emit
    response_encoder.{hpp,cpp}      zlib, container chosen by coding
    encoding_negotiator.{hpp,cpp}   Accept-Encoding -> one coding
  server/
    Server.{hpp,cpp}                listen/accept, component graph
    ThreadPool.{hpp,cpp}            worker pool
  utils/
    Logger.{hpp,cpp}                level-filtered stderr logging
    StringUtils.{hpp,cpp}           pure string helpers
```

Layering rule: `server → handlers → http → utils`. `compression` is a leaf
service that `handlers` calls; it never calls back.

## 6. How to add a route

Adding `GET /health` that returns `{"ok":true}`:

**1. Write the handler.** New file `src/handlers/routes/health_route.cpp`.
Signature is fixed — that is the point:

```cpp
// health_route.cpp - GET /health

#include "handlers/routes/route_handlers.hpp"

#include "http/HttpConstants.hpp"

namespace handlers::routes {

http::HttpResponse handleHealth(const RouteContext& /*ctx*/) {
    return http::HttpResponse::ok("{\"ok\":true}",
                                 http::HttpConstants::MIME_APPLICATION_JSON);
}

} // namespace handlers::routes
```

**2. Declare it** in `src/handlers/routes/route_handlers.hpp`:

```cpp
// GET /health
http::HttpResponse handleHealth(const RouteContext& ctx);
```

**3. Add one row** to `kRouteTable` in `route_registry.cpp`:

```cpp
{http::HttpConstants::METHOD_GET, "/health", &handleHealth},
```

**4. Add a HEAD row** so HEAD keeps working everywhere:

```cpp
{http::HttpConstants::METHOD_HEAD, "/health", asHead(&handleHealth)},
```

Done. No `if` was added, no existing function was edited beyond the table, and
compression, keep-alive, `Content-Length` and HEAD all follow automatically
because the dispatcher owns them.

Verify:

```sh
cmake -B build -S . && cmake --build ./build
./build/http-server --directory /tmp/files &
curl -i localhost:4221/health
curl -I localhost:4221/health          # 200, headers only
```

## 7. Do not edit these

- **`.codecrafters/compile.sh` and `.codecrafters/run.sh`** — these define how
  CodeCrafters builds and runs the program. `your_program.sh` mirrors them for
  local use; if you change one, change both.
- **`codecrafters.yml`** — `buildpack: cpp-23` is what selects the compiler.
- **`src/http/HttpConstants.hpp`** — the protocol vocabulary. Add status codes
  and methods as `constexpr` here. But do **not** hand-write encoding strings
  like `"deflate"` at a call site: use `compression::toHeaderValue(coding)`.
  The `ENCODING_*` constants are kept for reference and are deliberately
  unused, so the compression layer is the single source of truth for what goes
  on the wire.

## 8. Known limitations

Not on the course, deliberately absent: TLS, Range requests / 206,
multipart/form-data. `Transfer-Encoding: chunked` is likewise unsupported —
requests must use `Content-Length`. A zero-byte file under `/files/` returns
500, because `readFile()` cannot distinguish an empty file from a failed read.

## 9. Testing

From a clean checkout, one command, 150 assertions, no network:

```sh
./tests/run.sh                  # 73 unit + 77 behavioural
./tests/run.sh unit             # just the C++ unit binaries
./tests/run.sh integration      # just the socket-level behaviour suite
ctest --test-dir build          # same suites, via CTest
```

The integration suite binds port 4221, which the server hardcodes and cannot be
moved. It **refuses to run** if that port is already busy rather than sharing
it with a stale server — if you see that message, `ss -tlnp | grep 4221`.

What each suite covers, and — more usefully — what it deliberately does not,
is in `tests/README.md`. Read it before trusting a green run. The two gaps
worth knowing about: response-header *values* other than `Content-Length` and
`Content-Encoding` are not asserted, and the 500 on a zero-byte file is a known
limitation that the suite deliberately does not pin down.

Two behaviours these suites exist to protect, both of which have broken before:

- **Pipelining with a body larger than the read buffer.** The old read loop
  appended whole 8192-byte reads, so a 20 KB body swallowed the request behind
  it. `test_pipelining.py` sends both in a single `write()` and requires two
  clean responses.
- **deflate must be a raw stream.** `test_compression.py` inflates every
  compressed body back with `zlib` and compares it to the original file, using
  `-MAX_WBITS` for deflate and `16+MAX_WBITS` for gzip. A correct
  `Content-Encoding` header over a broken body is still broken.
