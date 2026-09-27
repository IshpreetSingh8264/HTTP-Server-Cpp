# 🍛 HTTP Dhaba Server - Pinglish Edition

[![progress-banner](https://backend.codecrafters.io/progress/http-server/445bc82a-2d71-49fc-afc5-a57e2f6e1f24)](https://app.codecrafters.io/users/codecrafters-bot?r=2qF)

> **Sat Sri Akaal!** An HTTP/1.1 server built from scratch in C++23 — raw BSD
> sockets, a thread pool, and zlib — with humor, love, and lots of Pinglish
> comments!

This is a solution to the ["Build Your Own HTTP server" Challenge](https://app.codecrafters.io/courses/http-server/overview) from CodeCrafters. **All 14 stages pass** (verified with `codecrafters test`; see [Verification](#-verification) below).

## 🌟 Features

### Course stages ✅

All 14 CodeCrafters stages, verified passing:

| # | Stage | Slug |
|---|---|---|
| 1 | Bind to a port | `at4` |
| 2 | Respond with 200 | `ia4` |
| 3 | Extract URL path | `ih0` |
| 4 | Respond with body | `cn2` |
| 5 | Read header | `fs3` |
| 6 | Concurrent connections | `ej5` |
| 7 | Return a file | `ap6` |
| 8 | Read request body | `qv8` |
| 9 | Compression headers | `df4` |
| 10 | Multiple compression schemes | `ij8` |
| 11 | Gzip compression | `cr8` |
| 12 | Persistent connections | `ag9` |
| 13 | Concurrent persistent connections | `ul1` |
| 14 | Connection closure | `kh7` |

### Beyond the course ✅

- ✅ **gzip *and* raw deflate** — both produced, negotiated per `Accept-Encoding`
- ✅ **Content-coding negotiation** — `q` weights, `q=0` rejection, `*` wildcard, client order on ties
- ✅ **Persistent connections** — HTTP/1.1 keep-alive with a 30 s idle timeout
- ✅ **HTTP pipelining** — several requests in one TCP segment are framed correctly
- ✅ **HEAD** — real headers-only responses, including the `Content-Length` a GET would have sent
- ✅ **405 Method Not Allowed** — a known path with the wrong method is not a 400
- ✅ **Path traversal protection** — every path canonicalised into the served directory
- ✅ **Strict `Content-Length` parsing** — `-5`, `abc` and out-of-range values are rejected with a 400
- ✅ **Request size cap** — 1 MB
- ✅ **Signal handling** — clean shutdown on `SIGINT`/`SIGTERM`
- ✅ **Quiet by default** — logs to stderr at `ERROR`; `--verbose` for development

### Not implemented, on purpose

TLS, Range requests / 206, and `multipart/form-data` were **removed from the
CodeCrafters curriculum** and are deliberately absent. `Transfer-Encoding:
chunked` is also unsupported: request bodies must use `Content-Length`.

## 🏗️ Architecture

```
┌─────────────┐
│   Client    │
└──────┬──────┘
       │ HTTP Request
       ▼
┌──────────────────────────────────────────────────────────────┐
│  server::Server   listen/accept, owns the component graph    │
└──────┬───────────────────────────────────────────────────────┘
       │
       ▼
┌──────────────────────────────────────────────────────────────┐
│  server::ThreadPool   one task per accepted connection       │
└──────┬───────────────────────────────────────────────────────┘
       │
       ▼
┌──────────────────────────────────────────────────────────────┐
│  handlers::ConnectionHandler                                 │
│  • reads exactly ONE request, keeps the rest buffered         │
│  • keep-alive loop · 30s timeout · 1MB cap                    │
└──────┬───────────────────────────────────────────────────────┘
       │  http::HttpRequest
       ▼
┌──────────────────────────────────────────────────────────────┐
│  handlers::RouteHandler   thin dispatcher                    │
└──────┬───────────────────────────────────────────────────────┘
       │
       ▼
┌──────────────────────────────────────────────────────────────┐
│  handlers::routes   THE ROUTE TABLE (data, not control flow) │
│    GET    /                 root_route.cpp                   │
│    GET    /echo/{str}       echo_route.cpp                   │
│    GET    /user-agent       user_agent_route.cpp             │
│    GET    /files/{name...}  files_route.cpp                  │
│    POST   /files/{name...}  files_route.cpp                  │
│    HEAD   <each GET>        head_adapter.cpp                 │
│  └── no method match → 405 · no path match → 404              │
└──────┬──────────────────────────────┬────────────────────────┘
       │                              │
       ▼                              ▼
┌────────────────────────┐  ┌──────────────────────────────┐
│ compression::          │  │ handlers::FileHandler        │
│  EncodingNegotiator   │  │  • read / write              │
│  ResponseEncoder      │  │  • path-traversal sandbox     │
│  • gzip  (RFC 1952)   │  └──────────────────────────────┘
│  • deflate (RFC 1951) │
└────────────────────────┘
```

`src/main.cpp` is 72 lines: parse two flags, install a signal handler, construct
`Server`, start. No domain logic.

## 📁 Project Structure

Headers live next to their translation units under `src/`, so `src` is the
include root and `#include "http/HttpRequest.hpp"` resolves from anywhere.

```
codecrafters-http-server-cpp/
├── .github/
│   └── copilot-instructions.md  # architecture doc — start here
├── docs/
│   ├── ARCHITECTURE.md          # system design & flow  (see note below)
│   ├── LEARNING_GUIDE.md        # deep-dive on the concepts
│   └── API_REFERENCE.md         # class/method docs     (see note below)
├── src/
│   ├── main.cpp                 # thin entry point
│   ├── http/                    # HttpRequest, HttpResponse, HttpConstants
│   ├── handlers/                # ConnectionHandler, FileHandler, RouteHandler
│   │   └── routes/              # the route table + one file per endpoint
│   ├── compression/             # content_encoding, response_encoder, negotiator
│   ├── server/                  # Server, ThreadPool
│   └── utils/                   # Logger, StringUtils
├── CMakeLists.txt
└── vcpkg.json                   # pthreads, zlib
```

Every header has a matching `.cpp`; there are no header-only classes and no
empty translation units. Namespaces map one-to-one onto directories:
`http`, `server`, `handlers`, `compression`, `utils`.

> **Note on `docs/`** — the prose in those three files is still accurate as
> teaching material, but their file paths, class names and code excerpts
> predate the restructure. Treat
> [`.github/copilot-instructions.md`](.github/copilot-instructions.md) as the
> authoritative architecture reference.

## 🚀 Quick Start

### Prerequisites

- **C++23 compiler** (GCC 11+, Clang 14+)
- **CMake 3.13+**
- **vcpkg** (for dependency management)
- **zlib** (for gzip compression)
- **pthreads** (for threading)

### Build

```bash
# Set up vcpkg (if not already done)
export VCPKG_ROOT=/path/to/vcpkg

# Configure with CMake
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake

# Build
cmake --build ./build

# Or use the provided script
./your_program.sh
```

### Run

```bash
# Default: port 4221, files served from the current directory, quiet logging
./build/http-server

# Serve files from a specific directory
./build/http-server --directory /path/to/files

# Same, but with DEBUG logging on stderr
./build/http-server --directory /path/to/files --verbose
```

### Test with curl

```bash
# Test root endpoint
curl http://localhost:4221/

# Test echo endpoint
curl http://localhost:4221/echo/hello

# Test user-agent
curl http://localhost:4221/user-agent

# Test file serving
echo "Hello World" > test.txt
curl http://localhost:4221/files/test.txt

# Test file upload
curl -X POST http://localhost:4221/files/newfile.txt -d "File content here"

# Test HEAD (headers only, no body)
curl -I http://localhost:4221/echo/hello

# Test gzip compression
curl -H "Accept-Encoding: gzip" --compressed http://localhost:4221/echo/hello

# Test deflate compression
curl -H "Accept-Encoding: deflate" --compressed http://localhost:4221/echo/hello

# Test content-coding negotiation: the first coding the client lists wins
curl -sI -H "Accept-Encoding: deflate, gzip" http://localhost:4221/echo/hello | grep -i content-encoding
#   -> Content-Encoding: deflate
curl -sI -H "Accept-Encoding: gzip, deflate" http://localhost:4221/echo/hello | grep -i content-encoding
#   -> Content-Encoding: gzip
curl -sI -H "Accept-Encoding: gzip;q=0.1, deflate;q=0.9" http://localhost:4221/echo/hello | grep -i content-encoding
#   -> Content-Encoding: deflate   (higher q wins)

# Test a 405
curl -i -X PUT http://localhost:4221/echo/hello
#   -> HTTP/1.1 405 Method Not Allowed

# Test persistent connections
curl -v http://localhost:4221/ http://localhost:4221/echo/test
```

## 🎯 Supported HTTP Routes

| Method | Path | Description | Example |
|--------|------|-------------|---------|
| GET | `/` | Root endpoint | `curl http://localhost:4221/` |
| GET | `/echo/{str}` | Echo back the string | `curl http://localhost:4221/echo/hello` |
| GET | `/user-agent` | Return the `User-Agent` header | `curl http://localhost:4221/user-agent` |
| GET | `/files/{name}` | Serve a file from the directory | `curl http://localhost:4221/files/test.txt` |
| POST | `/files/{name}` | Save a file to the directory | `curl -X POST http://localhost:4221/files/new.txt -d "content"` |
| HEAD | any of the above | Headers only, no body | `curl -I http://localhost:4221/echo/hello` |

Routing is a data table in
[`src/handlers/routes/route_registry.cpp`](src/handlers/routes/route_registry.cpp).
Anything else is `404 Not Found`; a known path with an unlisted method is
`405 Method Not Allowed`. See
[`.github/copilot-instructions.md`](.github/copilot-instructions.md) for a
worked "how to add a route" recipe.

## 📚 Documentation

- **[LEARNING_GUIDE.md](docs/LEARNING_GUIDE.md)** - Comprehensive guide covering:
  - TCP/IP & Socket Programming
  - HTTP Protocol Internals
  - Concurrency & Threading
  - Compression Algorithms
  - Persistent Connections

- **[ARCHITECTURE.md](docs/ARCHITECTURE.md)** - System design:
  - Module interactions
  - Request lifecycle
  - Thread pool design
  - Error handling

- **[API_REFERENCE.md](docs/API_REFERENCE.md)** - Complete API documentation:
  - Class descriptions
  - Method signatures
  - Usage examples

## 🎨 Pinglish Comments

This project features hilarious **Pinglish** (Punjabi + English) comments throughout:

```cpp
// Oye! Socket ni baneya, koi problem hai
// (Hey! Socket didn't create, there's a problem)

// Gzip ne kamaal kar ditta! 
// (Gzip did wonders!)

// Client aa gaya ji, swaagat hai!
// (Client arrived, welcome!)
```

## 🧪 Testing with CodeCrafters

```sh
codecrafters test        # runs all 14 stages
git push origin master   # or: codecrafters submit
```

## ✅ Verification

Every claim on this page was checked by running the thing, not by reading it.

**All 14 CodeCrafters stages pass.** `codecrafters test`, 2026-09-27:

```
[tester::#KH7] Test passed.   (Connection closure)
[tester::#UL1] Test passed.   (Concurrent persistent connections)
[tester::#AG9] Test passed.   (Persistent connections)
[tester::#CR8] Test passed.   (Gzip compression)
[tester::#IJ8] Test passed.   (Multiple compression schemes)
[tester::#DF4] Test passed.   (Compression headers)
[tester::#QV8] Test passed.   (Read request body)
[tester::#AP6] Test passed.   (Return a file)
[tester::#EJ5] Test passed.   (Concurrent connections)
[tester::#FS3] Test passed.   (Read header)
[tester::#CN2] Test passed.   (Respond with body)
[tester::#IH0] Test passed.   (Extract URL path)
[tester::#IA4] Test passed.   (Respond with 200)
[tester::#AT4] Test passed.   (Bind to a port)
Test passed. Congrats!
```

`ij8` is worth spelling out, because its tester is weaker than the stage name
suggests. It only asserts that a `Content-Encoding` header is *present* for
`Accept-Encoding: encoding-1, gzip, encoding-2` and *absent* for
`Accept-Encoding: encoding-1, encoding-2`. It never checks *which* coding comes
back, and it never mentions deflate. So the stage passed before deflate
existed, and it passes now that deflate is really implemented and negotiated.

**Compression, measured** on a 400-byte body, with an independent zlib
round-trip check:

| Request | Response | Size |
|---|---|---|
| *(no `Accept-Encoding`)* | — | 400 |
| `Accept-Encoding: gzip` | `Content-Encoding: gzip` | 26 |
| `Accept-Encoding: deflate` | `Content-Encoding: deflate` | 8 |
| `Accept-Encoding: gzip, deflate` | `Content-Encoding: gzip` | 26 |
| `Accept-Encoding: deflate, gzip` | `Content-Encoding: deflate` | 8 |
| `Accept-Encoding: gzip;q=0.1, deflate;q=0.9` | `Content-Encoding: deflate` | 8 |
| `Accept-Encoding: identity` | *(no header)* | 400 |
| `Accept-Encoding: br` | *(no header)* | 400 |

Both `gzip -d` and a raw-DEFLATE `zlib.decompressobj(-MAX_WBITS)` recover the
original bytes, and `curl --compressed` round-trips both.

**Also verified with curl:** 200/404/405/400-when-malformed status codes,
`PUT`/`DELETE`/`OPTIONS` → 405, `HEAD` → 200 with `Content-Length: 3` and 0
body bytes, file upload and read-back, byte-identical binary serving,
path-traversal rejection, 10 concurrent connections, keep-alive, and four
pipelining cases (including a 20 KB body that spans three `recv` calls).

## 🔧 Configuration

### Thread Pool Size

Defaults to `std::thread::hardware_concurrency()` workers, minimum 4. To pin
it, change the construction in
[Server.cpp](src/server/Server.cpp):

```cpp
threadPool_ = std::make_unique<ThreadPool>(8);
```

### Connection Timeout

Default **30 seconds** (the HTTP/1.1 recommendation). Change
`TIMEOUT_SECONDS` in
[ConnectionHandler.hpp](src/handlers/ConnectionHandler.hpp).

### Max Request Size

Default **1 MB** (`MAX_REQUEST_SIZE`, same file). A larger `Content-Length` is
rejected with a 400 rather than buffered.

### Port Number

Default **4221**, the `DEFAULT_PORT` constant in
[main.cpp](src/main.cpp).

## 🛡️ Security Features

- **Path Traversal Protection** — every path is canonicalised and rejected
  unless it resolves inside the served directory. A plain string-prefix check
  would let `/data/root2` through a `/data/root` base, so the check compares
  whole path components.
- **Request Size Limits** — 1 MB, and a `Content-Length` that is negative,
  non-numeric, or out of range is rejected before a single body byte is read
- **Timeout Protection** — 30 s read timeout
- **Safe File Operations** — every file operation goes through the sandbox

## 📚 Documentation

- **[.github/copilot-instructions.md](.github/copilot-instructions.md)** —
  **the authoritative architecture reference**: component map, data flow,
  conventions, module map, and a worked "how to add a route" recipe

- **[ARCHITECTURE.md](docs/ARCHITECTURE.md)** — system design: module
  interactions, request lifecycle, thread pool design, error handling.
  *Paths and class names predate the restructure; see the note in
  [Project Structure](#-project-structure).*

- **[LEARNING_GUIDE.md](docs/LEARNING_GUIDE.md)** — the concepts behind it all:
  TCP/IP and socket programming, HTTP protocol internals, concurrency and
  threading, compression algorithms, persistent connections. *Same caveat.*

- **[API_REFERENCE.md](docs/API_REFERENCE.md)** — class and method walkthroughs.
  *Same caveat — it still documents the old header-only layout.*

## 🤝 Contributing

This is a learning project, and the Pinglish comments are deliberate. Real
improvements are very welcome:

- Keep `main.cpp` thin and routing data-driven — see
  [copilot-instructions.md](.github/copilot-instructions.md) §6
- Every header needs a matching `.cpp`, and no file should pass 600 lines
- Prove non-obvious behaviour with a test, not a comment
- Fix the `docs/` staleness if you touch the code it describes

## 📝 License

For educational purposes (the CodeCrafters challenge).

## 🙏 Acknowledgments

- **CodeCrafters** — for the challenge, and for a public
  `course-definition.yml` that made the stage list verifiable
- **Punjabi culture** — for the humour and the warmth
- **the C++ community** — for the tools and the libraries

---

**Sat Sri Akaal! Happy coding! 🚀**
