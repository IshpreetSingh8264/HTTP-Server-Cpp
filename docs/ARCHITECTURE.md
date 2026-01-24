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
│    │ FileHandler  │            │ GzipCompressor    │     │
│    │ • Read files │            │ • Compress data   │     │
│    │ • Write files│            │ • Check encoding  │     │
│    │ • Path safety│            │ • Detect support  │     │
│    └──────────────┘            └───────────────────┘     │
│                                                            │
└────────────────────────────────────────────────────────────┘
```

### Layer Responsibilities

| Layer | Responsibility | Key Classes |
|-------|----------------|-------------|
| **Entry** | Application startup, config | `main.cpp` |
| **Server** | Socket lifecycle, orchestration | `Server`, `ThreadPool` |
| **Handler** | Request processing, routing | `ConnectionHandler`, `RouteHandler`, `FileHandler` |
| **Protocol** | HTTP parsing & building | `HttpRequest`, `HttpResponse`, `HttpConstants` |
| **Compression** | Data compression | `GzipCompressor` |
| **Utilities** | Cross-cutting concerns | `Logger`, `StringUtils` |

---

## Module Architecture

### 1. Server Module (`include/server/`)

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

**Implementation:** [Server.hpp](../include/server/Server.hpp), [ThreadPool.hpp](../include/server/ThreadPool.hpp)

---

### 2. Handler Module (`include/handlers/`)

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
- **Responsibility:** URL routing, business logic
- **Routes:**
  | Pattern | Handler | Compression |
  |---------|---------|-------------|
  | `GET /` | `handleRoot()` | No |
  | `GET /echo/{str}` | `handleEcho()` | Yes |
  | `GET /user-agent` | `handleUserAgent()` | Yes |
  | `GET /files/{name}` | `handleFileGet()` | Text only |
  | `POST /files/{name}` | `handleFilePost()` | No |

**Routing Algorithm:**
```cpp
if (path == "/") return handleRoot();
if (startsWith(path, "/echo/")) return handleEcho();
if (path == "/user-agent") return handleUserAgent();
if (startsWith(path, "/files/")) {
    return (method == "GET") ? handleFileGet() : handleFilePost();
}
return notFound();
```

#### FileHandler
- **Responsibility:** File I/O, security
- **Security Features:**
  - Path traversal protection (canonical paths)
  - Base directory enforcement
  - Read/write validation

**Path Safety:**
```cpp
canonical_base = "/var/www/"
canonical_path = canonicalize(base + requested_path)

if (canonical_path starts with canonical_base) → SAFE
else → REJECT (path traversal attack!)
```

**Implementation:** [ConnectionHandler.hpp](../include/handlers/ConnectionHandler.hpp), [RouteHandler.hpp](../include/handlers/RouteHandler.hpp), [FileHandler.hpp](../include/handlers/FileHandler.hpp)

---

### 3. HTTP Module (`include/http/`)

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

**Implementation:** [HttpRequest.hpp](../include/http/HttpRequest.hpp), [HttpResponse.hpp](../include/http/HttpResponse.hpp), [HttpConstants.hpp](../include/http/HttpConstants.hpp)

---

### 4. Compression Module (`include/compression/`)

**Purpose:**
> Data compression lai bandwidth bachana
>
> (For data compression to save bandwidth)

**Components:**

#### GzipCompressor
- **Compression Pipeline:**
  ```
  Input data
      ↓
  deflateInit2() - Initialize zlib with gzip format
      ↓
  deflate() - Compress
      ↓
  deflateEnd() - Cleanup
      ↓
  Compressed data
  ```

**Decision Tree:**
```
Accept-Encoding has "gzip"?
  ├─No──> Don't compress
  └─Yes─> Data size > 1KB?
            ├─No──> Don't compress (overhead > benefit)
            └─Yes─> Content-Type compressible?
                      ├─No (image/video)──> Don't compress
                      └─Yes (text/*)────> COMPRESS!
```

**Implementation:** [GzipCompressor.hpp](../include/compression/GzipCompressor.hpp)

---

### 5. Utilities Module (`include/utils/`)

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

**Implementation:** [Logger.hpp](../include/utils/Logger.hpp), [StringUtils.hpp](../include/utils/StringUtils.hpp)

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

**Implementation:** [ThreadPool.hpp](../include/server/ThreadPool.hpp)

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
    
    // Route-specific logic
    HttpResponse res = routeSpecificHandler(req);
    
    // Common post-processing
    applyCompression(res, req);
    
    return res;
}
```

### 4. Strategy Pattern
**Used in:** Compression

**Purpose:** Different compression strategies (gzip, deflate, none)

**Example:**
```cpp
if (supportsGzip(acceptEncoding)) {
    strategy = GzipCompressor::compress;
} else if (supportsDeflate(acceptEncoding)) {
    strategy = DeflateCompressor::compress;
} else {
    strategy = identity;  // No compression
}
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
