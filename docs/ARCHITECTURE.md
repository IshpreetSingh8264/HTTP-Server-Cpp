# 🏗️ HTTP Server Architecture

> **System design te data flow - sab kuch organized!**
>
> (System design and data flow - everything organized!)

This document describes the architecture, module interactions, and design patterns used in the HTTP Dhaba Server.

---

## Table of Contents

1. [System Overview](#system-overview)
2. [Module Architecture](#module-architecture)
3. [Request Lifecycle](#request-lifecycle)
4. [Thread Pool Design](#thread-pool-design)
5. [Error Handling Strategy](#error-handling-strategy)
6. [Design Patterns](#design-patterns)

---

## System Overview

### High-Level Architecture

```
┌──────────────────── HTTP Dhaba Server ────────────────────┐
│                                                            │
│  ┌─────────────────── Entry Point ────────────────────┐   │
│  │  main.cpp                                          │   │
│  │  • Parse command-line args (--directory)           │   │
│  │  • Setup signal handlers (Ctrl+C)                  │   │
│  │  • Create Server instance                          │   │
│  └──────────────────────┬─────────────────────────────┘   │
│                         │                                  │
│  ┌──────────────────────▼──────────────────────────────┐  │
│  │  Server (Orchestrator)                              │  │
│  │  • Socket management (create, bind, listen)         │  │
│  │  • Accept loop (main thread)                        │  │
│  │  • Dispatch connections to ThreadPool               │  │
│  │  • Graceful shutdown                                │  │
│  └──────────┬─────────────────────────┬────────────────┘  │
│             │                         │                    │
│     ┌───────▼────────┐       ┌────────▼────────┐          │
│     │  ThreadPool    │       │  RouteHandler   │          │
│     │  • N workers   │       │  • URL routing  │          │
│     │  • Task queue  │       │  • Compression  │          │
│     │  • Concurrency │       │  • Responses    │          │
│     └───────┬────────┘       └────────┬────────┘          │
│             │                         │                    │
│     ┌───────▼─────────────────────────▼────────┐          │
│     │  ConnectionHandler                       │          │
│     │  • Read HTTP request                     │          │
│     │  • Persistent connection loop            │          │
│     │  • Timeout management                    │          │
│     │  • Send HTTP response                    │          │
│     └──────────────────┬───────────────────────┘          │
│                        │                                   │
│         ┌──────────────┴────────────────┐                 │
│         │                               │                 │
│    ┌────▼─────────┐            ┌────────▼──────────┐     │
│    │ FileHandler  │            │  compression::    │     │
│    │ • Read files │            │   Negotiator      │     │
│    │ • Write files│            │   • parse A-E     │     │
│    │ • Path safety│            │   • pick coding   │     │
│    │              │            │  Encoder          │     │
│    │              │            │   • gzip / deflate│     │
│    └──────────────┘            └───────────────────┘     │
│                                                            │
└────────────────────────────────────────────────────────────┘
```

### Layer Responsibilities

| Layer | Responsibility | Key Classes |
|-------|----------------|-------------|
| **Entry** | Application startup, config | `main.cpp` |
| **Server** | Socket lifecycle, orchestration | `Server`, `ThreadPool` |
| **Handler** | Request framing, routing, file IO | `ConnectionHandler`, `RouteHandler`, `FileHandler` |
| **Protocol** | HTTP parsing & building | `HttpRequest`, `HttpResponse`, `HttpConstants` |
| **Compression** | Coding negotiation and encoding | `EncodingNegotiator`, `ResponseEncoder`, `ContentEncoding` |
| **Utilities** | Cross-cutting concerns | `Logger`, `StringUtils` |

---

## Module Architecture

### 1. Server Module (`src/server/`)

**Purpose:**
> Server de core functionality - socket setup te connection management
>
> (Server's core functionality - socket setup and connection management)

**Components:**

#### Server Class
- **Responsibility:** Main orchestrator
- **Lifecycle:**
  1. Create socket (`socket()`)
  2. Set options (`setsockopt()`)
  3. Bind to port (`bind()`)
  4. Listen (`listen()`)
  5. Accept loop (`accept()`)
  6. Dispatch to ThreadPool
  7. Graceful shutdown

**State Machine:**
```
   INIT
     │
     ▼
  CREATED ──socket()──> BOUND ──listen()──> LISTENING ──accept()──> RUNNING
     │         │          │                      │
     └─error───┴──────────┴──────────────────────┴─────────> STOPPED
```

#### ThreadPool Class
- **Responsibility:** Manage worker threads
- **Architecture:** Producer-Consumer pattern
  - **Producer:** Server's accept loop (adds tasks)
  - **Consumer:** Worker threads (execute tasks)
  - **Queue:** `std::queue<std::function<void()>>`
  - **Synchronization:** `std::mutex` + `std::condition_variable`

**Data Flow:**
```
accept() → enqueue(task) → notify_one() → worker picks task → execute
```

**Implementation:** [Server.hpp](../src/server/Server.hpp), [ThreadPool.hpp](../src/server/ThreadPool.hpp)

---

### 2. Handler Module (`src/handlers/`)

**Purpose:**
> Requests nu handle karo - connection se lekar response tak
>
> (Handle requests - from connection to response)

**Components:**

#### ConnectionHandler
- **Responsibility:** Socket I/O, persistent connections
- **Flow:**
  ```
  1. Set socket timeout (30s)
  2. Loop while keep-alive:
     a. Read request (recv())
     b. Parse request
     c. Route to handler
     d. Send response (send())
     e. Check Connection header
  3. Close socket
  ```

**Persistent Connection Logic:**
```cpp
bool keep_alive = true;
while (keep_alive) {
    request = read_request();
    response = route_handler->handle(request);
    
    // Check if client wants to close
    if (request.getHeader("Connection") == "close") {
        keep_alive = false;
        response.setHeader("Connection", "close");
    }
    
    send_response(response);
}
```

#### RouteHandler
- **Responsibility:** thin dispatcher. It holds the `FileHandler` and hands the
  request to the route table in `handlers/routes/`. It contains no routing
  logic of its own.
- **Routes** (the table lives in [route_registry.cpp](../src/handlers/routes/route_registry.cpp)):

  | Pattern | Handler | File |
  |---------|---------|------|
  | `GET /` | `handleRoot()` | `root_route.cpp` |
  | `GET /echo/{str}` | `handleEcho()` | `echo_route.cpp` |
  | `GET /user-agent` | `handleUserAgent()` | `user_agent_route.cpp` |
  | `GET /files/{name...}` | `handleFileGet()` | `files_route.cpp` |
  | `POST /files/{name...}` | `handleFilePost()` | `files_route.cpp` |
  | `HEAD` on each of the above | `asHead(<getHandler>)` | `head_adapter.cpp` |

  Compression is no longer a per-route decision. The dispatcher applies the
  content-coding policy to whatever the handler returned, so a route cannot
  forget to compress and a new route gets compression for free.

**Routing algorithm:** iterate the table, match the pattern, compare the
method.
```cpp
for (const Route& route : routeTable()) {
    if (!pathMatches(path, route.pathPattern)) continue;
    pathMatchedSomeRoute = true;
    if (method == route.method) return route.handler(ctx);
}
return pathMatchedSomeRoute ? methodNotAllowed() : notFound();
```

Pattern syntax: `{name}` matches exactly one path segment, `{name...}` is a
greedy tail that matches the rest. A known path with an unlisted method is
**405**, not 400; an unknown path is **404**.

#### FileHandler
- **Responsibility:** File I/O, security
- **Security Features:**
  - Path traversal protection (canonical paths)
  - Base directory enforcement
  - Read/write validation

**Path Safety:**
```cpp
canonical_base = canonical(baseDirectory)
canonical_path = weakly_canonical(baseDirectory / requested)

// Compare whole path COMPONENTS, not characters: a character-prefix check
// would accept "/var/wwwroot" for a base of "/var/www".
mismatch(base.begin(), base.end(), path.begin(), path.end());
if (base_iter == base.end()) → SAFE
else → REJECT (path traversal)
```

**Implementation:** [ConnectionHandler.hpp](../src/handlers/ConnectionHandler.hpp), [RouteHandler.hpp](../src/handlers/RouteHandler.hpp), [FileHandler.hpp](../src/handlers/FileHandler.hpp)

---

### 3. HTTP Module (`src/http/`)

**Purpose:**
> HTTP protocol lai - parsing te building
>
> (For HTTP protocol - parsing and building)

**Components:**

#### HttpRequest
- **Parsing Pipeline:**
  ```
  Raw string → Split by "\r\n\r\n" → Headers + Body
                    │
                    ├─> Request line → Method, Path, Version
                    └─> Header lines → Map<Name, Value>
  ```

**State Diagram:**
```
  EMPTY
    │
    ├─parse()─> PARSING ─success─> VALID
    │               │
    │               └─fail─> INVALID
    │
    └─isValid()─> VALID / INVALID
```

#### HttpResponse
- **Building Pipeline:**
  ```
  Status Code → Status Line
      ↓
  Headers → Add headers
      ↓
  Body → Calculate Content-Length
      ↓
  toString() → Complete HTTP response
  ```

**Factory Methods:**
- `ok(body)` → 200 OK
- `created()` → 201 Created
- `notFound()` → 404 Not Found
- `internalError()` → 500 Error

#### HttpConstants
- **Centralized Constants:**
  - Status codes + text
  - Header names
  - MIME types
  - Protocol strings

**Implementation:** [HttpRequest.hpp](../src/http/HttpRequest.hpp), [HttpResponse.hpp](../src/http/HttpResponse.hpp), [HttpConstants.hpp](../src/http/HttpConstants.hpp)

---

### 4. Compression Module (`src/compression/`)

**Purpose:**
> Data compression lai bandwidth bachana
>
> (For data compression to save bandwidth)

**Components:**

Three classes, one per concept. Before the restructure this was a single
`GzipCompressor` that only ever produced gzip, despite a `deflate` constant
sitting unused in `HttpConstants`.

| Class | Responsibility |
|---|---|
| `ContentEncoding` | the codings we can emit (`Gzip`, `Deflate`, `Identity`) |
| `ResponseEncoder` | zlib deflate, container chosen by coding |
| `EncodingNegotiator` | `Accept-Encoding` -> one chosen coding |

**Encoding Pipeline:**
```
Negotiation:  Accept-Encoding -> parse -> score each coding -> pick one
                                                ↓
Encoder:       deflateInit2(windowBits)  →  deflate(Z_FINISH)  →  deflateEnd()
                 15 + 16 = gzip container
                 -15     = raw deflate stream
```

Both codings are the same DEFLATE algorithm; only the container differs. On a
400-byte body gzip costs 26 bytes and deflate costs 8 — the 18-byte difference
is exactly the gzip header and trailer.

**Decision Tree:**
```
Accept-Encoding present?
  ├─No──> Identity (no compression)
  └─Yes─> score every coding we can produce:
            highest q wins;  coding;q=0 is refused
            on a q tie, the client's list order wins
            "*" accepts anything we can produce
              ↓
          Content-Type already compressed (image/video/audio/zip)?
            ├─Yes──> Identity
            └─No───> compress with the chosen coding
```

**Implementation:** [content_encoding.hpp](../src/compression/content_encoding.hpp),
[response_encoder.hpp](../src/compression/response_encoder.hpp),
[encoding_negotiator.hpp](../src/compression/encoding_negotiator.hpp)

---

### 5. Utilities Module (`src/utils/`)

**Purpose:**
> Cross-cutting concerns - logging, string operations
>
> (Common utilities across all modules)

**Components:**

#### Logger
- **Thread-Safe Logging:**
  ```
  log() → lock_guard<mutex> → colorize → output → unlock
  ```

- **Log Levels:**
  - DEBUG (Cyan) - Detailed info
  - INFO (Green) - Normal operations
  - WARN (Yellow) - Warnings
  - ERROR (Red) - Errors

#### StringUtils
- **Utilities:**
  - `split()` - Split strings
  - `trim()` - Remove whitespace
  - `toLower()` - Lowercase conversion
  - `equalsIgnoreCase()` - Case-insensitive compare
  - `urlDecode()` - URL decoding

**Implementation:** [Logger.hpp](../src/utils/Logger.hpp), [StringUtils.hpp](../src/utils/StringUtils.hpp)

---

## Request Lifecycle

### Complete Flow Diagram

```
Client                    Server                  ThreadPool              ConnectionHandler         RouteHandler          FileHandler
  │                         │                         │                          │                        │                    │
  ├─TCP Connect────────────>│                         │                          │                        │                    │
  │                         ├─accept()                │                          │                        │                    │
  │                         ├─enqueue(task)──────────>│                          │                        │                    │
  │                         │                         ├─worker picks task        │                        │                    │
  │                         │                         ├─execute()───────────────>│                        │                    │
  │                         │                         │                          ├─setSocketTimeout()     │                    │
  ├─HTTP Request───────────────────────────────────────────────────────────────>│                        │                    │
  │                         │                         │                          ├─readRequest()          │                    │
  │                         │                         │                          ├─parse()                │                    │
  │                         │                         │                          ├─handleRequest()───────>│                    │
  │                         │                         │                          │                        ├─route matching     │
  │                         │                         │                          │                        ├─handleFileGet()───>│
  │                         │                         │                          │                        │                    ├─readFile()
  │                         │                         │                          │                        │<───file data───────┤
  │                         │                         │                          │                        ├─applyCompression() │
  │                         │                         │                          │<───HttpResponse────────┤                    │
  │                         │                         │                          ├─sendResponse()         │                    │
  │<─HTTP Response───────────────────────────────────────────────────────────────┤                        │                    │
  │                         │                         │                          ├─check Connection       │                    │
  │                         │                         │                          ├─keep-alive? Loop      │                    │
  │                         │                         │                          │   OR                   │                    │
  │                         │                         │                          ├─close? End            │                    │
  ├─TCP Close──────────────────────────────────────────────────────────────────>│                        │                    │
  │                         │                         │                          ├─closeConnection()      │                    │
```

### Step-by-Step Breakdown

1. **Connection Acceptance** (Server)
   - `accept()` blocks waiting for client
   - Returns client socket FD
   - Enqueues task to ThreadPool

2. **Task Dispatch** (ThreadPool)
   - Worker thread picks task from queue
   - Executes `handleClient(socket)`

3. **Request Reading** (ConnectionHandler)
   - Sets 30s timeout
   - Reads until `\r\n\r\n` found
   - Reads body if `Content-Length` present

4. **Request Parsing** (HttpRequest)
   - Splits request line
   - Parses headers into map
   - Validates format

5. **Routing** (RouteHandler)
   - Matches URL pattern
   - Calls appropriate handler
   - Applies compression if needed

6. **File Operations** (FileHandler) - if file route
   - Validates path safety
   - Reads/writes file
   - Returns data

7. **Response Building** (HttpResponse)
   - Sets status code
   - Adds headers
   - Compresses body if applicable

8. **Response Sending** (ConnectionHandler)
   - Sends via `send()`
   - Checks `Connection` header
   - Loops or closes

---

## Thread Pool Design

### Architecture

```
┌─────────────── ThreadPool ───────────────┐
│                                          │
│  ┌────────── Task Queue ──────────┐     │
│  │  std::queue<function<void()>>  │     │
│  │                                 │     │
│  │  [Task 1] [Task 2] [Task 3]    │     │
│  └────────────┬────────────────────┘     │
│               │                          │
│  ┌────────────▼───────────────┐          │
│  │  Mutex + Condition Var     │          │
│  │  • Lock queue access       │          │
│  │  • Signal workers          │          │
│  └────────────┬───────────────┘          │
│               │                          │
│  ┌────────────▼────────────┐             │
│  │  Worker Threads         │             │
│  │  ┌─────┐ ┌─────┐       │             │
│  │  │ T1  │ │ T2  │ ... │ TN│            │
│  │  └─────┘ └─────┘       │             │
│  └─────────────────────────┘             │
│                                          │
└──────────────────────────────────────────┘
```

### Worker Thread Lifecycle

```
START
  │
  ▼
WAIT (condition.wait)
  │
  ├─notified─> CHECK (stop || !tasks.empty())
  │               │
  │               ├─stop && empty─> TERMINATE
  │               │
  │               └─!empty────────> GET TASK
  │                                    │
  │                                    ▼
  │                                 EXECUTE
  │                                    │
  └────────────────────────────────────┘
  (Loop back to WAIT)
```

### Synchronization Details

**Enqueue Operation:**
```cpp
{
    lock_guard<mutex> lock(queue_mutex);  // LOCK
    tasks.push(task);                      // MODIFY
}                                          // UNLOCK
condition.notify_one();                    // SIGNAL
```

**Worker Operation:**
```cpp
{
    unique_lock<mutex> lock(queue_mutex);        // LOCK
    condition.wait(lock, predicate);             // WAIT (releases lock)
    // Woken up + re-acquired lock
    task = tasks.front();                        // GET
    tasks.pop();                                 // REMOVE
}                                                // UNLOCK
task();                                          // EXECUTE (outside lock!)
```

**Implementation:** [ThreadPool.hpp](../src/server/ThreadPool.hpp)

---

## Error Handling Strategy

### Layers of Defense

1. **Input Validation**
   - HTTP request parsing
   - Path traversal checks
   - Size limits

2. **Exception Handling**
   - Try-catch in critical sections
   - Graceful degradation
   - Error responses to client

3. **Resource Management**
   - RAII (constructors/destructors)
   - Smart pointers
   - Socket cleanup

4. **Logging**
   - All errors logged with context
   - Pinglish humor for readability

### Error Response Strategy

```cpp
try {
    // Normal operation
    return handleRequest();
} catch (const std::filesystem::filesystem_error& e) {
    // File system error
    return HttpResponse::notFound("File not found");
} catch (const std::exception& e) {
    // Generic error
    Logger::error("Exception: " + string(e.what()));
    return HttpResponse::internalError("Server error");
} catch (...) {
    // Unknown error
    Logger::error("Unknown exception!");
    return HttpResponse::internalError("Unknown error");
}
```

---

## Design Patterns

### 1. Factory Pattern
**Used in:** `HttpResponse`

**Purpose:** Create response objects easily

**Example:**
```cpp
// Instead of:
HttpResponse res(200);
res.setContentType("text/plain");
res.setBody("OK");

// Use factory:
HttpResponse res = HttpResponse::ok("OK");
```

### 2. Builder Pattern
**Used in:** `HttpResponse`

**Purpose:** Build complex response step-by-step

**Example:**
```cpp
HttpResponse response;
response.setStatus(200);
response.setHeader("Content-Type", "text/html");
response.setHeader("Cache-Control", "no-cache");
response.setBody(html_content);
std::string output = response.toString();
```

### 3. Template Method Pattern
**Used in:** `RouteHandler`

**Purpose:** Common request handling flow, custom route logic

**Template:**
```cpp
HttpResponse handleRequest(HttpRequest& req) {
    // Common pre-processing
    log(req);
    
    // Route-specific logic - the matched table row
    HttpResponse res = matchedRoute.handler(ctx);
    
    // Common post-processing - owned here, not by each route (Rule 8)
    applyContentEncoding(res, req);
    
    return res;
}
```

### 4. Registry / Table-Driven Dispatch
**Used in:** Routing, and content-coding selection

**Purpose:** Add a route or a content coding as one row of data, not as a new
branch in a growing if/else.

**Routing example** — the entire mechanism, from `routes/route_registry.cpp`:
```cpp
const std::vector<Route> kRouteTable = {
    {METHOD_GET,  "/",                &handleRoot},
    {METHOD_GET,  "/echo/{str}",      &handleEcho},
    {METHOD_GET,  "/files/{name...}", &handleFileGet},
    {METHOD_POST, "/files/{name...}", &handleFilePost},
    {METHOD_HEAD, "/",                asHead(&handleRoot)},
    // ...
};
```

**Content-coding example** — same idea, applied to `Accept-Encoding`:
```cpp
// The candidate set is data, so adding a coding needs no new control flow.
constexpr ContentEncoding kOffered[] = {ContentEncoding::Gzip, ContentEncoding::Deflate};
```

### 5. Producer-Consumer Pattern
**Used in:** `ThreadPool`

**Purpose:** Decouple connection acceptance from handling

**Components:**
- **Producer:** Server's accept loop
- **Consumer:** Worker threads
- **Queue:** Task queue
- **Synchronization:** Mutex + Condition Variable

---

## Performance Considerations

### 1. Thread Pool Sizing
- **Formula:** `num_threads = hardware_concurrency()`
- **Rationale:** Match CPU cores for optimal CPU utilization
- **Trade-off:** More threads = more concurrency, but higher context switching

### 2. Buffer Sizes
- **Request buffer:** 8KB (typical HTTP request)
- **File read:** 8KB chunks
- **Trade-off:** Larger = fewer system calls, but more memory

### 3. Compression Threshold
- **Minimum size:** 1KB
- **Rationale:** Overhead > benefit for small files
- **Compression ratio:** ~60-80% for text

### 4. Connection Timeout
- **Value:** 30 seconds
- **Rationale:** HTTP/1.1 standard, balance between keep-alive and resource usage

---

## Scalability

### Current Limits
- **Concurrent connections:** Limited by thread pool size (~CPU cores)
- **Request size:** 1MB max
- **File size:** Unlimited (streaming)

### Future Improvements
- **Event-driven I/O:** epoll/kqueue for >10K connections
- **HTTP/2:** Multiplexing multiple requests on one connection
- **Load balancing:** Multiple server instances

---

**Architecture complete! Server tayar hai!**

**(Architecture complete! Server is ready!)**
