# HTTP Server — Architecture

How the server is put together. The [README](../README.md) covers what it does; this covers the shape of the code.

> This file replaces `docs/API_REFERENCE.md` and `docs/LEARNING_GUIDE.md`. The API reference documented a different
> codebase: it gave `FileHandler::readFile` a `std::string` return and said it throws, when it returns a
> `std::vector<char>` and never throws; it gave `ConnectionHandler` a `handleClient(int)` method and no socket in its
> constructor, when it has `handleConnection()` and takes the fd; and it described `Server::stop()` as waiting for
> active connections, which it does not do.

## Contents

- [Layering](#layering)
- [The route table](#the-route-table)
- [Path matching](#path-matching)
- [Connection lifecycle](#connection-lifecycle)
- [Content encoding as a policy](#content-encoding-as-a-policy)
- [The file sandbox](#the-file-sandbox)
- [Header storage](#header-storage)
- [Adding a route](#adding-a-route)
- [File map](#file-map)
- [Conventions](#conventions)

## Layering

```
main.cpp
  └── server/Server                 listening socket, accept loop
        └── server/ThreadPool       fixed worker pool
              └── handlers/ConnectionHandler    the per-connection loop
                    ├── handlers/RouteHandler   a forward to routes::dispatch
                    │     └── handlers/routes/  the table, matching, dispatch, encoding
                    │           └── handlers/FileHandler
                    ├── compression/            zlib + negotiation
                    ├── http/                   HttpRequest, HttpResponse
                    └── utils/                  StringUtils, Logger
```

Each layer knows only the one below it. `ConnectionHandler` does not know which routes exist; the registry does not know
about sockets; `FileHandler` knows nothing about HTTP.

`CMakeLists.txt` compiles everything except `main.cpp` into `libhttpcore.a`, so the unit tests link the same objects the
server does.

## The route table

```cpp
struct Route {
  const char* method;
  const char* pattern;
  RouteHandlerFn handler;
};
```

`route_registry.cpp` holds a `constexpr` array of nine entries and a `table()` accessor. Four are `GET` routes, one is
`POST /files/...`, and four are `HEAD` rows that wrap a `GET` handler through `asHead()`.

`asHead` is a small adapter, not a duplicate table: it runs the underlying handler, then flips
`HttpResponse::setHeadersOnly(true)`. The body is still built, so `Content-Length` stays truthful — only the
serialisation changes.

`dispatch()` is the whole routing policy:

```cpp
if (path && method match)  { response = handler(ctx); applyContentEncoding(response, request); return response; }
if (path matches)          { return HttpResponse::methodNotAllowed(...); }   // 405
                           { return HttpResponse::notFound(...); }           // 404
```

## Path matching

`pathMatches` splits both path and pattern on `/` using `StringUtils::split`, which **preserves empty tokens**. That is
load-bearing: it is why `"/"` yields `["", ""]` rather than `[]`, and why a request line containing a double space is
rejected by `parseRequestLine` rather than silently accepted.

Two parameter forms:

- `{name}` — positional, matches exactly one segment. Does not swallow `/`, so `/echo/a/b` is a `404`.
- `{name...}` — greedy tail. Returns true as soon as it is reached, so nothing after it is re-checked.

A literal route compares length first, so `/user-agency` cannot match `/user-agent`.

`--name-only` style flags are handled by scanning the argument list in the command file, not by the matcher. The matcher
knows about patterns only.

## Connection lifecycle

`ConnectionHandler::handleConnection` is a loop:

```cpp
while (true) {
  auto read = readRequest(buffer_);        // ReadResult: Ok | Invalid | Closed
  if (read == Invalid) { send 400; break; }
  if (read == Closed)  break;

  HttpRequest request;
  request.parse(carved);

  HttpResponse response = routeHandler_->handleRequest(request);
  sendResponse(response);

  if (clientWantsClose(request)) break;    // Connection: close, case-insensitive
}
```

The `Connection` header is inspected **before** the response is sent, so the decision to keep the socket open is made
with full knowledge of what the client asked for.

`readBuffer_` is a member, not a local. That is what makes pipelining work: a second request already sitting in the
buffer is picked up on the next iteration with no extra `recv`.

## Content encoding as a policy

`applyContentEncoding` lives in an anonymous namespace in `route_registry.cpp` and is called from `dispatch` — not from
inside the routes. Three consequences worth naming:

1. **A route never has to think about compression.** It returns a plain response; the policy layer decides.
2. **HEAD responses report the length their GET would have had**, because the body is still built before `asHead` strips
   it from the output.
3. **The decision is media-type only.** `shouldCompress` never looks at whether the bytes are compressible, and there is
   no size threshold. `application/octet-stream` is not on the skip list, so a random binary file *is* gzipped — and
   still round-trips correctly. That is asserted in the integration suite.

A zlib failure falls back to identity. An empty result from `compress` is therefore ambiguous between "identity" and
"zlib failed", which is safe only because the dispatcher never calls it for identity.

## The file sandbox

```cpp
bool isPathSafe(const path& candidate) {
  auto base = std::filesystem::canonical(baseDirectory_);
  auto full = std::filesystem::weakly_canonical(candidate);
  return std::mismatch(base, full).first == base.end();
}
```

`mismatch` over **path components** rather than characters. That is the difference between rejecting
`/data/root2` against a base of `/data/root` (correct) and a character-prefix comparison, which would accept it.

`weakly_canonical` on the candidate is deliberate: the file being served usually does not exist yet on a `POST`, and
`canonical` would throw. Any exception is caught and turned into a denial, so a weird path fails closed.

`readFile` and `writeFile` return `{}` / `false` on every failure and never throw. That is why the routes map failures
to status codes explicitly rather than catching — the error information is the empty return, not an exception.

## Header storage

`HttpRequest::headers_` and `HttpResponse::headers_` are both `unordered_map<string, string>` — one value per name, and
`HttpResponse` additionally emits them in unspecified order.

Request lookups (`getHeader`, `hasHeader`) are **case-insensitive**, done by linear scan with `equalsIgnoreCase`.
Response lookups are **case-sensitive**, using `unordered_map::find`. The asymmetry is real and currently harmless,
because every internal call site uses the same `HttpConstants` string. It would bite anyone adding a route that mixes
cases.

`HttpRequest::parseHeaderLine` comma-joins a repeated name rather than overwriting it, per RFC 9110 §5.3. The
`unordered_map` would otherwise give last-one-wins, which silently drops an `Accept-Encoding` offer.

## Adding a route

1. Write a handler in `src/handlers/routes/<name>_route.cpp` returning an `HttpResponse`.
2. Declare it in `src/handlers/routes/route_handlers.hpp`.
3. Add a row to the array in `route_registry.cpp`.

Add a `HEAD` row too, via `asHead`, if the route should answer `HEAD`. There is no automatic derivation.

`routeCount()` exists and is asserted in the unit test, so a forgotten row fails a test rather than shipping silently.

## File map

| Path | Responsibility |
|---|---|
| `src/main.cpp` | `--directory`, `--verbose`, signal handlers, `DEFAULT_PORT` |
| `src/server/Server.cpp` | socket setup, `acceptLoop`, `stop` |
| `src/server/ThreadPool.cpp` | worker construction, queue, shutdown |
| `src/handlers/ConnectionHandler.cpp` | keep-alive loop, framing, `sendAll` |
| `src/handlers/RouteHandler.cpp` | forwards to `routes::dispatch` |
| `src/handlers/routes/route_registry.cpp` | route table, matcher, dispatch, encoding policy |
| `src/handlers/routes/head_adapter.cpp` | `asHead` |
| `src/handlers/FileHandler.cpp` | the sandbox, read, write |
| `src/compression/response_encoder.cpp` | gzip and raw deflate, skip list |
| `src/compression/encoding_negotiator.cpp` | `Accept-Encoding` parsing and choice |
| `src/compression/content_encoding.cpp` | header-value spelling |
| `src/http/HttpRequest.cpp` | parsing, header merge, case-insensitive lookup |
| `src/http/HttpResponse.cpp` | factories, serialisation, HEAD mode |
| `src/http/HttpConstants.cpp` | status text, MIME guessing |

## Conventions

- **Every header has a matching `.cpp`, with one exception.** `routes/route_handlers.hpp` is declaration-only; its
  functions live in the per-route files.
- **Bilingual Punjabi/English file headers** across `src/`, though many files are largely English inside.
- **Diagnostics to stderr, results to stdout.** The integration suite parses stdout, so nothing else may write there.
- **No third-party HTTP library.** zlib for compression, POSIX for everything else. `vcpkg.json` exists because the
  CodeCrafters image expects it, but the local build uses system zlib.
- **Errors are status codes, not exceptions.** Handlers return a `HttpResponse`; nothing above `FileHandler` throws.
- **Tests link `libhttpcore.a`**, so a unit test can never pass against a divergent copy of the code.
