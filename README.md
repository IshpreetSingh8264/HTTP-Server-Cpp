# HTTP Server — C++

An HTTP/1.1 server in C++23, built for the
[CodeCrafters "Build your own HTTP server" challenge](https://codecrafters.io/challenges/http-server).

Nine routes, a fixed worker pool, persistent connections, pipelining, and gzip plus deflate with real
`Accept-Encoding` negotiation. Raw POSIX sockets, no HTTP framework.

> **Scope note.** HTTP/1.1 over plain TCP, IPv4 only, no TLS. Chunked request bodies, `Range`, redirects, and
> HTTP/2 are not implemented — see [Not implemented](#not-implemented).

## Contents

- [Quick start](#quick-start)
- [Routes](#routes)
- [Architecture](#architecture)
- [Concurrency](#concurrency)
- [Request framing](#request-framing)
- [Compression](#compression)
- [File serving](#file-serving)
- [Tests](#tests)
- [Not implemented](#not-implemented)
- [Project layout](#project-layout)

## Quick start

Requires **CMake 3.20+** (C++23 support landed in 3.20), a C++23 compiler, zlib, and Python 3. Python is a hard
configure-time requirement, not just a test dependency.

```bash
sudo apt install zlib1g-dev cmake

cmake -B build -S .
cmake --build ./build
./build/http-server
```

Or:

```bash
./your_program.sh
```

The port is **fixed at 4221** and cannot be changed — there is no `--port` flag.

```bash
$ curl -i localhost:4221/
HTTP/1.1 200 OK
Connection: keep-alive
Content-Length: 0
Content-Type: text/plain

$ curl localhost:4221/echo/hello
hello
$ curl -H 'User-Agent: my-client/1.0' localhost:4221/user-agent
my-client/1.0
$ curl --data-binary 'contents' localhost:4221/files/note.txt -X POST
$ curl localhost:4221/files/note.txt
contents
```

## Routes

| Method | Path | Behaviour |
|---|---|---|
| `GET` | `/` | `200`, empty body, `text/plain`. |
| `GET` | `/echo/{str}` | `200`, echoes the single path segment verbatim. No URL decoding. |
| `GET` | `/user-agent` | `200`, the `User-Agent` value, or the literal `Unknown`. |
| `GET` | `/files/{name...}` | `200` with the file's bytes and a guessed `Content-Type`. |
| `POST` | `/files/{name...}` | `201` on success. `400` on an empty body. |
| `HEAD` | `/`, `/echo/{str}`, `/user-agent`, `/files/{name...}` | Same headers as the `GET`, no body. |

`{str}` matches exactly one segment, so `/echo/a/b` is a `404`. `{name...}` is a greedy tail, so `/files/a/b/c.txt`
works and names `a/b/c.txt`.

Resolution order: path and method both match → handler; path matches but method does not → `405`; nothing matches →
`404`.

## Architecture

```
  accept()                                    ┌──────────────────────┐
     │                                        │  routes::dispatch    │
     ▼                                        │                      │
 ThreadPool.enqueue(...)                      │  1. pathMatches      │
     │                                        │  2. handler          │
     ▼                                        │  3. applyContent     │
 ┌──────────────────────────────┐              │       Encoding       │
 │  ConnectionHandler           │──────────────│  4. serialise        │
 │                              │  Request     └──────────────────────┘
 │  loop {                      │  Response        │
 │    readRequest  ── framing ──┤                 │
 │    dispatch                 │            route table
 │    sendResponse ── partial ──┘                 │
 │  }                            │          /  /echo/{str}
 └──────────────────────────────┘          /  /user-agent
            │                             /  /files/{name...}
            ▼                             + 4 HEAD rows
      FileHandler
      (sandboxed)
```

| Module | Responsibility |
|---|---|
| `server/Server.cpp` | listening socket, accept loop |
| `server/ThreadPool.cpp` | fixed worker pool |
| `handlers/ConnectionHandler.cpp` | the per-connection loop, framing, sending |
| `handlers/routes/route_registry.cpp` | the route table, matching, dispatch, encoding policy |
| `handlers/FileHandler.cpp` | the only component that touches the filesystem |
| `compression/` | zlib encode, `Accept-Encoding` negotiation |
| `http/` | `HttpRequest`, `HttpResponse`, constants |
| `utils/` | string helpers, logging |

`RouteHandler` is a two-line forward to `routes::dispatch`. The real work is in the registry.

## Concurrency

A fixed pool sized to `hardware_concurrency()`, with a floor of 4. The main thread blocks in `accept()` and hands each
accepted fd to the pool as a task; the worker runs `handleClient` for the connection's entire lifetime.

```cpp
ThreadPool(size_t numThreads = 0);   // 0 → hardware_concurrency(), minimum 4
void enqueue(Task);                  // drops the task silently if the pool is stopping
```

Workers execute outside the mutex and catch everything, so one bad request cannot kill a worker. The destructor sets
the stop flag, notifies everyone, and joins.

**The ceiling is N connections, not N plus a queue.** A task holds its socket for the whole connection, including every
keep-alive iteration, and each `recv()` can block for up to 30 seconds. So with four workers, a fifth connection sits
in the queue and its client sees nothing until a worker frees up. This is a real limit, not a theoretical one.

All sockets are blocking. `SO_RCVTIMEO` is set to 30 seconds per connection; `SO_SNDTIMEO` is **not** set, so writing
to a peer that has stopped reading can block indefinitely. `TCP_NODELAY` is not set either.

`SIGPIPE` is handled by passing `MSG_NOSIGNAL` on every `send`, rather than by ignoring the signal globally.

## Request framing

Framing is where HTTP servers usually get subtle, so it is worth spelling out.
`ConnectionHandler::readRequest`:

1. Find `\r\n\r\n` in the persistent read buffer. Everything after it is the body.
2. Parse a throwaway `HttpRequest` from the buffer just to read `Content-Length`.
3. Validate it — digits only, no sign, no exponent, `ERANGE` or above the maximum is a failure. A failure is a `400`
   followed by a close.
4. If the buffer holds header + `Content-Length` bytes, carve out exactly that many and leave the rest for the next
   iteration. This is what makes pipelining work.
5. Otherwise `recv()` into an 8 KiB stack buffer and loop.

`parseContentLength` exists because three real bugs shipped through the naive version: a non-numeric value threw out of
`readRequest` and killed the worker silently, a negative value wrapped to about 1.8 × 10¹⁹ and the server waited forever
for a body that would never arrive, and an absurd exponent threw `out_of_range`.

Two limits to know about:

- An oversized *raw* byte stream gets no response at all. The client sees EOF, not a `400`. Only a bad or oversized
  *declared* `Content-Length` produces a `400`.
- The effective request ceiling is `MAX_REQUEST_SIZE` (1 MiB) **minus the header bytes**, because the total is checked
  against the same limit. A `Content-Length` of exactly 1 MiB therefore passes validation and then fails to buffer.

## Compression

Both codings are the same DEFLATE; only the container differs.

- `gzip` — RFC 1952 framing, `deflateInit2` with `windowBits = 15 + 16`.
- `deflate` — **raw** RFC 1951, `windowBits = -15`. RFC 9110 nominally specifies the zlib/RFC 1950 wrapper here, but
  every browser and curl expect the raw stream, so that is what is sent.

**Negotiation** (`EncodingNegotiator`) parses `Accept-Encoding` into preferences with `q` values, skips codings the
server does not offer, and picks the highest `q`. Ties break on the client's ordering. `q=0` is a refusal. `*` matches
anything. An empty or whitespace-only header means `Identity` — a server that compresses unasked breaks naive clients.
A `406` is never returned.

**Policy** is applied in `dispatch`, after the handler returns and before serialisation — not inside the routes, so a
route never has to think about it. `ResponseEncoder::shouldCompress` skips empty bodies and anything whose content type
contains `image/`, `video/`, `audio/`, `application/zip`, or `application/gzip`. There is deliberately **no minimum
size threshold**, because the test harness expects compression even on tiny bodies. A failed zlib call falls back to
identity rather than failing the request.

**Repeated headers are comma-joined.** RFC 9110 §5.3 says a list-valued field that appears more than once is the
comma-joined union of every occurrence. Without that, this:

```bash
curl -H 'Accept-Encoding: deflate' -H 'Accept-Encoding: gzip' ...
```

would silently lose the `deflate` offer, because the header map is a plain `unordered_map` and last-one-wins is the
default. So `HttpRequest::parseHeaderLine` appends `", " + value` when a name is already present.

Two caveats worth knowing. The merge keys on the raw, un-normalised name, so identically-cased field names merge but
differently-cased ones do not. And the merge is unconditional, so a repeated `Content-Length` becomes `"5, 10"` and is
then correctly rejected as non-numeric — right outcome, accidental mechanism.

## File serving

`FileHandler` is the only component that touches the filesystem, and every path goes through `isPathSafe`:

```cpp
canonicalBase = std::filesystem::canonical(baseDirectory_);
canonicalPath = std::filesystem::weakly_canonical(path);
safe = std::mismatch(base, path) puts baseEnd at canonicalBase.end();
```

The comparison is **component-wise**, not character-wise, so `/data/root2` is correctly rejected against a base of
`/data/root`. Any exception during resolution denies the request.

Files are read whole into memory; nothing is streamed. Writes create parent directories, so a nested `POST` works.

| Situation | Status |
|---|---|
| `GET`, file exists, non-empty | `200` |
| `GET`, missing or unsafe path | `404` |
| `GET`, zero-byte file | `500` |
| `POST`, empty body | `400` |
| `POST`, write failed or unsafe path | `500` |
| `POST`, success | `201` |

The zero-byte `500` is odd but long-standing and called out in the source: you cannot create a zero-byte file with
`POST` (that is a `400`) and you cannot read one back. A `POST` traversal attempt returns `500` rather than `403`.

## Tests

```bash
./tests/run.sh                # configure, build, all suites, size budget
./tests/run.sh unit           # the three C++ binaries
./tests/run.sh integration    # behavioural suite; needs port 4221 free

cmake -B build -S . && cmake --build build && ctest --test-dir build --output-on-failure
```

**150 assertions: 73 unit, 77 integration.**

| Suite | Assertions | Covers |
|---|---|---|
| `unit/route_matcher_test.cpp` | 12 | path matching, both parameter forms, 404 vs 405 |
| `unit/negotiation_test.cpp` | 35 | `q` values, case, whitespace, `*`, aliases, malformed input |
| `unit/string_utils_test.cpp` | 26 | `split` including empty tokens, and the string helpers |
| `integration/test_routing.py` | 37 | raw sockets: status codes, HEAD, pipelining, `Content-Length` validation |
| `integration/test_compression.py` | 31 | a 14-row negotiation table plus single-shot encode/decode round trips |
| `integration/test_pipelining.py` | 9 | POST-then-GET, 20 KB bodies, keep-alive, 10 concurrent connections |

All three unit binaries link `libhttpcore.a`, so they exercise the shipped code rather than a copy. The integration
suite speaks raw sockets — no curl — and starts the server against a temporary directory, so nothing depends on the
machine's real filesystem.

`tests/check_size.py` fails if any test file passes 500 lines.

**Not covered**, and stated in `tests/README.md`: TLS, `Range` and `206`, multipart, chunked request bodies, the
zero-byte-file `500`, response headers other than `Content-Length` and `Content-Encoding`, the thread pool directly,
slow clients, and load. The repeated-`Accept-Encoding` merge is also untested.

## Not implemented

- **No chunked request bodies.** A `Transfer-Encoding: chunked` body is treated as zero bytes.
- **No `Vary: Accept-Encoding`.** A caching intermediary can hand a gzipped body to a client that asked for `identity`.
- **No `Date` or `Server` header**, and no `Allow` header on a `405`.
- **No redirects.** There is no 3xx status in the constants table.
- **No `Expect: 100-continue`.**
- **HTTP/1.0 is parsed but ignored.** The version is stored and never used, so an HTTP/1.0 request with no `Connection`
  header gets keep-alive.
- **No TLS, no HTTP/2, no upgrade.**
- **The port cannot be changed**, and the bind is IPv4-only.
- **Shutdown is not graceful.** `stop()` flips a flag and closes the listening socket; in-flight connections are not
  drained, and the signal handler calls `_exit` directly.
- **Response header order is unspecified** — headers are emitted from an `unordered_map`.
- **`HttpResponse::getHeader` is case-sensitive** while `HttpRequest::getHeader` is not. It works today only because
  every call site uses the same constants.

## Project layout

```
src/
  main.cpp                  argument parsing, signal handlers
  server/                   Server, ThreadPool
  handlers/                 ConnectionHandler, RouteHandler, FileHandler
  handlers/routes/          the route table and one file per route
  compression/              zlib encoding and Accept-Encoding negotiation
  http/                     HttpRequest, HttpResponse, HttpConstants
  utils/                    StringUtils, Logger
tests/
  unit/                     three C++ suites
  integration/              three Python suites and a shared harness
  run.sh, check_size.py
```

`CMakeLists.txt` puts everything except `main.cpp` into `libhttpcore.a`, so the unit tests link the shipped code.
`src` is the include root, hence role-based includes like `#include "http/HttpRequest.hpp"`.

## Licence

No licence file is present in this repository. Add one before redistributing.
