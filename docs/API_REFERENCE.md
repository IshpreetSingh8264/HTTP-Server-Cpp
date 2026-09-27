# 📚 API Reference

> **Complete API documentation - har function ki details!**
>
> (Complete API documentation - details of every function!)

This document provides comprehensive API documentation for all classes, methods, and functions in the HTTP Dhaba Server.

---

## Table of Contents

1. [Utilities](#utilities)
   - [Logger](#logger)
   - [StringUtils](#stringutils)
2. [HTTP Module](#http-module)
   - [HttpConstants](#httpconstants)
   - [HttpRequest](#httprequest)
   - [HttpResponse](#httpresponse)
3. [Compression](#compression)
   - [ContentEncoding](#contentencoding)
   - [EncodingNegotiator](#encodingnegotiator)
   - [ResponseEncoder](#responseencoder)
4. [Handlers](#handlers)
   - [FileHandler](#filehandler)
   - [RouteHandler](#routehandler)
   - [ConnectionHandler](#connectionhandler)
5. [Server](#server)
   - [ThreadPool](#threadpool)
   - [Server](#server-1)

---

## Utilities

### Logger

**File:** [src/utils/Logger.hpp](../src/utils/Logger.hpp)

**Purpose:** Thread-safe logging system with Pinglish messages and color support.

#### Methods

##### `static void debug(const std::string& message)`

Logs debug-level message (cyan color).

**Parameters:**
- `message` - The debug message to log

**Example:**
```cpp
Logger::debug("Request parsing shuru ho gaya");
```

**Thread Safety:** ✅ Yes (mutex-protected)

---

##### `static void info(const std::string& message)`

Logs informational message (green color).

**Parameters:**
- `message` - The info message to log

**Example:**
```cpp
Logger::info("Server 4221 te ready hai!");
```

**Thread Safety:** ✅ Yes

---

##### `static void warn(const std::string& message)`

Logs warning message (yellow color).

**Parameters:**
- `message` - The warning message to log

**Example:**
```cpp
Logger::warn("Compression fail, sending uncompressed");
```

**Thread Safety:** ✅ Yes

---

##### `static void error(const std::string& message)`

Logs error message (red color).

**Parameters:**
- `message` - The error message to log

**Example:**
```cpp
Logger::error("Socket bind fail ho gaya!");
```

**Thread Safety:** ✅ Yes

---

### StringUtils

**File:** [src/utils/StringUtils.hpp](../src/utils/StringUtils.hpp)

**Purpose:** String manipulation utilities for HTTP parsing.

#### Methods

##### `static std::vector<std::string> split(const std::string& str, char delimiter)`

Splits a string by delimiter.

**Parameters:**
- `str` - String to split
- `delimiter` - Character to split on

**Returns:** Vector of split strings

**Example:**
```cpp
auto parts = StringUtils::split("GET /hello HTTP/1.1", ' ');
// parts = ["GET", "/hello", "HTTP/1.1"]
```

---

##### `static std::string trim(const std::string& str)`

Removes leading and trailing whitespace.

**Parameters:**
- `str` - String to trim

**Returns:** Trimmed string

**Example:**
```cpp
std::string trimmed = StringUtils::trim("  hello  ");
// trimmed = "hello"
```

---

##### `static std::string toLower(const std::string& str)`

Converts string to lowercase.

**Parameters:**
- `str` - String to convert

**Returns:** Lowercase string

**Example:**
```cpp
std::string lower = StringUtils::toLower("Content-Type");
// lower = "content-type"
```

---

##### `static bool equalsIgnoreCase(const std::string& str1, const std::string& str2)`

Case-insensitive string comparison.

**Parameters:**
- `str1` - First string
- `str2` - Second string

**Returns:** `true` if equal (ignoring case), `false` otherwise

**Example:**
```cpp
bool equal = StringUtils::equalsIgnoreCase("GZIP", "gzip");
// equal = true
```

---

##### `static bool startsWith(const std::string& str, const std::string& prefix)`

Checks if string starts with prefix.

**Parameters:**
- `str` - String to check
- `prefix` - Prefix to match

**Returns:** `true` if starts with prefix, `false` otherwise

**Example:**
```cpp
bool starts = StringUtils::startsWith("/files/hello.txt", "/files/");
// starts = true
```

---

##### `static bool endsWith(const std::string& str, const std::string& suffix)`

Checks if string ends with suffix.

**Parameters:**
- `str` - String to check
- `suffix` - Suffix to match

**Returns:** `true` if ends with suffix, `false` otherwise

**Example:**
```cpp
bool ends = StringUtils::endsWith("index.html", ".html");
// ends = true
```

---

##### `static std::string urlDecode(const std::string& str)`

Decodes URL-encoded string.

**Parameters:**
- `str` - URL-encoded string

**Returns:** Decoded string

**Example:**
```cpp
std::string decoded = StringUtils::urlDecode("hello%20world");
// decoded = "hello world"
```

---

## HTTP Module

### HttpConstants

**File:** [src/http/HttpConstants.hpp](../src/http/HttpConstants.hpp)

**Purpose:** Centralized HTTP protocol constants.

#### Constants

```cpp
// Status codes
static const int STATUS_OK = 200;
static const int STATUS_CREATED = 201;
static const int STATUS_BAD_REQUEST = 400;
static const int STATUS_NOT_FOUND = 404;
static const int STATUS_INTERNAL_SERVER_ERROR = 500;

// Headers
static const std::string HEADER_CONTENT_TYPE = "Content-Type";
static const std::string HEADER_CONTENT_LENGTH = "Content-Length";
static const std::string HEADER_CONTENT_ENCODING = "Content-Encoding";
static const std::string HEADER_ACCEPT_ENCODING = "Accept-Encoding";
static const std::string HEADER_CONNECTION = "Connection";
static const std::string HEADER_USER_AGENT = "User-Agent";
```

#### Methods

##### `static std::string getStatusText(int statusCode)`

Gets HTTP status text for code.

**Parameters:**
- `statusCode` - HTTP status code (200, 404, etc.)

**Returns:** Status text ("OK", "Not Found", etc.)

**Example:**
```cpp
std::string text = HttpConstants::getStatusText(404);
// text = "Not Found"
```

---

##### `static std::string getMimeType(const std::string& filename)`

Determines MIME type from file extension.

**Parameters:**
- `filename` - Filename with extension

**Returns:** MIME type string

**Example:**
```cpp
std::string mime = HttpConstants::getMimeType("index.html");
// mime = "text/html"
```

**Supported Types:**
- `.html`, `.htm` → `text/html`
- `.txt` → `text/plain`
- `.css` → `text/css`
- `.js` → `application/javascript`
- `.json` → `application/json`
- `.png` → `image/png`
- `.jpg`, `.jpeg` → `image/jpeg`
- `.gif` → `image/gif`
- Default → `application/octet-stream`

---

### HttpRequest

**File:** [src/http/HttpRequest.hpp](../src/http/HttpRequest.hpp)

**Purpose:** Parse and represent HTTP requests.

#### Constructor

```cpp
HttpRequest();
```

Creates empty request.

---

#### Methods

##### `bool parse(const std::string& requestString)`

Parses HTTP request from string.

**Parameters:**
- `requestString` - Raw HTTP request

**Returns:** `true` if valid, `false` if parsing failed

**Example:**
```cpp
HttpRequest req;
bool valid = req.parse("GET /hello HTTP/1.1\r\nHost: localhost\r\n\r\n");
```

**Format Expected:**
```
METHOD PATH VERSION\r\n
Header1: Value1\r\n
Header2: Value2\r\n
\r\n
[Body]
```

---

##### `bool isValid() const`

Checks if request is valid.

**Returns:** `true` if parsed successfully, `false` otherwise

**Example:**
```cpp
if (req.isValid()) {
    // Process request
}
```

---

##### `std::string getMethod() const`

Gets HTTP method.

**Returns:** Method string (GET, POST, etc.)

**Example:**
```cpp
std::string method = req.getMethod();
// method = "GET"
```

---

##### `std::string getPath() const`

Gets request path.

**Returns:** URL path

**Example:**
```cpp
std::string path = req.getPath();
// path = "/files/hello.txt"
```

---

##### `std::string getVersion() const`

Gets HTTP version.

**Returns:** Version string (HTTP/1.1)

**Example:**
```cpp
std::string version = req.getVersion();
// version = "HTTP/1.1"
```

---

##### `std::string getHeader(const std::string& name) const`

Gets header value (case-insensitive).

**Parameters:**
- `name` - Header name

**Returns:** Header value, or empty string if not found

**Example:**
```cpp
std::string userAgent = req.getHeader("User-Agent");
```

---

##### `bool hasHeader(const std::string& name) const`

Checks if header exists (case-insensitive).

**Parameters:**
- `name` - Header name

**Returns:** `true` if exists, `false` otherwise

**Example:**
```cpp
if (req.hasHeader("Content-Length")) {
    // Body present
}
```

---

##### `std::string getBody() const`

Gets request body.

**Returns:** Body string

**Example:**
```cpp
std::string body = req.getBody();
```

---

### HttpResponse

**File:** [src/http/HttpResponse.hpp](../src/http/HttpResponse.hpp)

**Purpose:** Build HTTP responses.

#### Constructor

```cpp
HttpResponse(int statusCode = 200);
```

Creates response with status code.

**Parameters:**
- `statusCode` - HTTP status code (default 200)

---

#### Methods

##### `void setStatus(int statusCode)`

Sets status code.

**Parameters:**
- `statusCode` - HTTP status code

**Example:**
```cpp
response.setStatus(404);
```

---

##### `void setHeader(const std::string& name, const std::string& value)`

Sets response header.

**Parameters:**
- `name` - Header name
- `value` - Header value

**Example:**
```cpp
response.setHeader("Content-Type", "text/html");
```

---

##### `void setBody(const std::string& body)`

Sets response body (uncompressed).

**Parameters:**
- `body` - Response body

**Example:**
```cpp
response.setBody("<html>Hello</html>");
```

**Note:** Automatically sets Content-Length header.

---

##### `void setCompressedBody(const std::vector<char>& compressed, const std::string& encoding)`

Sets a pre-compressed response body.

**Parameters:**
- `compressed` - Compressed body bytes, from `ResponseEncoder`
- `encoding` - the `Content-Encoding` token, from `toHeaderValue(...)`

**Example:**
```cpp
auto coding = EncodingNegotiator::negotiate(req.getHeader("Accept-Encoding"));
auto body = ResponseEncoder::compress(payload, coding);
if (!body.empty()) {
    response.setCompressedBody(body, toHeaderValue(coding));
}
```

**Note:** Sets both `Content-Length` and `Content-Encoding`, and records
`isCompressed()`.

---

##### `void setHeadersOnly(bool headersOnly)`

Marks the response as a HEAD response: `toString()` then emits the status line
and headers but no body. The body is still built, so `Content-Length` reports
what the equivalent GET would have sent. Reached through
`asHead()` in `head_adapter.cpp`, never by a route directly.

---

##### `std::string toString() const`

Builds complete HTTP response string.

**Returns:** HTTP response ready to send

**Example:**
```cpp
std::string responseStr = response.toString();
send(socket, responseStr.c_str(), responseStr.size(), 0);
```

**Format:**
```
HTTP/1.1 200 OK\r\n
Content-Type: text/plain\r\n
Content-Length: 12\r\n
\r\n
Hello World!
```

---

#### Factory Methods

##### `static HttpResponse ok(const std::string& body = "")`

Creates 200 OK response.

**Parameters:**
- `body` - Response body (optional)

**Returns:** HttpResponse with 200 status

**Example:**
```cpp
HttpResponse res = HttpResponse::ok("Success!");
```

---

##### `static HttpResponse created()`

Creates 201 Created response.

**Returns:** HttpResponse with 201 status

**Example:**
```cpp
HttpResponse res = HttpResponse::created();
```

---

##### `static HttpResponse badRequest(const std::string& message = "")`

Creates 400 Bad Request response.

**Parameters:**
- `message` - Error message (optional)

**Returns:** HttpResponse with 400 status

**Example:**
```cpp
HttpResponse res = HttpResponse::badRequest("Invalid format");
```

---

##### `static HttpResponse notFound(const std::string& message = "")`

Creates 404 Not Found response.

**Parameters:**
- `message` - Error message (optional)

**Returns:** HttpResponse with 404 status

**Example:**
```cpp
HttpResponse res = HttpResponse::notFound("File not found");
```

---

##### `static HttpResponse internalError(const std::string& message = "")`

Creates 500 Internal Server Error response.

**Parameters:**
- `message` - Error message (optional)

**Returns:** HttpResponse with 500 status

**Example:**
```cpp
HttpResponse res = HttpResponse::internalError("Database error");
```

---

## Compression

### ContentEncoding

**File:** [src/compression/content_encoding.hpp](../src/compression/content_encoding.hpp)

**Purpose:** The content codings this server can emit.

```cpp
enum class ContentEncoding { Identity, Gzip, Deflate };
```

| Function | Returns |
|---|---|
| `toHeaderValue(ContentEncoding)` | the exact `Content-Encoding` token |
| `toAcceptEncodingToken(ContentEncoding)` | the `Accept-Encoding` token |
| `fromHeaderValue(std::string)` | `ContentEncoding`; unknown tokens become `Identity` |

---

### EncodingNegotiator

**File:** [src/compression/encoding_negotiator.hpp](../src/compression/encoding_negotiator.hpp)

**Purpose:** Turn an `Accept-Encoding` header into one `Content-Encoding`
value. This is the substance of CodeCrafters stage `ij8`.

##### `static ContentEncoding negotiate(const std::string& acceptEncoding)`

Priority order:

1. highest `q` wins; `coding;q=0` refuses that coding
2. on a `q` tie, the coding the client listed **first** wins
3. a bare `*` accepts any coding the server can produce
4. `identity` is the fallback, so an absent or unusable header yields an
   uncompressed response rather than a 406

**Example:**
```cpp
negotiate("gzip");                      // Gzip
negotiate("deflate");                   // Deflate
negotiate("gzip, deflate");             // Gzip   (first listed wins)
negotiate("deflate, gzip");             // Deflate
negotiate("gzip;q=0.1, deflate;q=0.9"); // Deflate (higher q wins)
negotiate("gzip;q=0");                  // Identity (refused)
negotiate("*");                         // Gzip (server preference)
negotiate("br");                        // Identity (unsupported coding)
negotiate("");                          // Identity (never compress unasked)
```

##### `static std::vector<EncodingPreference> parse(const std::string& acceptEncoding)`

Splits, lower-cases, and reads the `;q=` weights out of each element. Exposed
for diagnostics and tests.

---

### ResponseEncoder

**File:** [src/compression/response_encoder.hpp](../src/compression/response_encoder.hpp)

**Purpose:** Compress a response body with zlib. Both codings are the same
DEFLATE algorithm; they differ only in the container.

| Constant | `deflateInit2` windowBits | Container |
|---|---|---|
| `GZIP_WINDOW_BITS` | `15 + 16` | gzip, RFC 1952 |
| `DEFLATE_WINDOW_BITS` | `-15` | raw deflate, RFC 1951 |

##### `static std::vector<char> compress(const std::string& data, ContentEncoding encoding)`

**Returns:** the compressed bytes, or an **empty vector** on failure. The
caller treats an empty result as "send it uncompressed" and never fails the
request over compression. `Identity` returns empty by definition.

**Example:**
```cpp
auto body = compression::ResponseEncoder::compress(payload, ContentEncoding::Deflate);
if (!body.empty()) {
    response.setCompressedBody(body, compression::toHeaderValue(ContentEncoding::Deflate));
}
```

##### `static bool shouldCompress(const std::string& data, const std::string& contentType)`

False for an empty body, and false for a content type that is already
compressed (`image/*`, `video/*`, `audio/*`, `application/zip`,
`application/gzip`) — those only grow.

There is deliberately **no minimum-size threshold**: the CodeCrafters harness
expects compression even on tiny bodies.

**Note on `deflate`.** RFC 9110 nominally defines the coding as the zlib
format (RFC 1950, a positive windowBits), but in practice every browser and
curl send and expect a raw RFC 1951 stream, which is what this produces.

---

## Handlers

### FileHandler

**File:** [src/handlers/FileHandler.hpp](../src/handlers/FileHandler.hpp)

**Purpose:** Safe file I/O with path traversal protection.

#### Constructor

```cpp
FileHandler(const std::string& baseDirectory = "");
```

Creates file handler with base directory.

**Parameters:**
- `baseDirectory` - Base directory for file operations (default: current dir)

**Example:**
```cpp
FileHandler handler("/var/www/files");
```

---

#### Methods

##### `std::string readFile(const std::string& relativePath)`

Reads file content safely.

**Parameters:**
- `relativePath` - Relative path from base directory

**Returns:** File content as string

**Throws:** `std::runtime_error` if:
- Path is unsafe (traversal attempt)
- File doesn't exist
- Read error occurs

**Example:**
```cpp
std::string content = handler.readFile("hello.txt");
```

**Security:** Checks path traversal using `isPathSafe()`

---

##### `void writeFile(const std::string& relativePath, const std::string& content)`

Writes file content safely.

**Parameters:**
- `relativePath` - Relative path from base directory
- `content` - Content to write

**Throws:** `std::runtime_error` if:
- Path is unsafe
- Write error occurs

**Example:**
```cpp
handler.writeFile("output.txt", "Hello World!");
```

---

##### `bool fileExists(const std::string& relativePath)`

Checks if file exists.

**Parameters:**
- `relativePath` - Relative path from base directory

**Returns:** `true` if exists, `false` otherwise

**Example:**
```cpp
if (handler.fileExists("config.json")) {
    // Load config
}
```

---

##### `size_t getFileSize(const std::string& relativePath)`

Gets file size in bytes.

**Parameters:**
- `relativePath` - Relative path from base directory

**Returns:** File size in bytes

**Throws:** `std::runtime_error` if file doesn't exist

**Example:**
```cpp
size_t size = handler.getFileSize("large.bin");
```

---

##### `void deleteFile(const std::string& relativePath)`

Deletes file safely.

**Parameters:**
- `relativePath` - Relative path from base directory

**Throws:** `std::runtime_error` if:
- Path is unsafe
- File doesn't exist
- Delete error occurs

**Example:**
```cpp
handler.deleteFile("temp.txt");
```

---

### RouteHandler

**File:** [src/handlers/RouteHandler.hpp](../src/handlers/RouteHandler.hpp)

**Purpose:** URL routing and request handling.

#### Constructor

```cpp
RouteHandler(std::shared_ptr<FileHandler> fileHandler);
```

Creates route handler with file handler.

**Parameters:**
- `fileHandler` - Shared pointer to FileHandler

**Example:**
```cpp
auto fileHandler = std::make_shared<FileHandler>("/var/www");
RouteHandler routes(fileHandler);
```

---

#### Methods

##### `HttpResponse handleRequest(const HttpRequest& request)`

Routes request to appropriate handler.

**Parameters:**
- `request` - Parsed HTTP request

**Returns:** HttpResponse

**Routes:**
| Path Pattern | Method | Handler |
|--------------|--------|---------|
| `/` | GET | `handleRoot()` |
| `/echo/{str}` | GET | `handleEcho()` |
| `/user-agent` | GET | `handleUserAgent()` |
| `/files/{name}` | GET | `handleFileGet()` |
| `/files/{name}` | POST | `handleFilePost()` |

**Example:**
```cpp
HttpResponse res = routes.handleRequest(req);
```

---

`RouteHandler` no longer contains per-method or per-path branches. The whole
routing mechanism is [routes/route_registry.cpp](../src/handlers/routes/route_registry.cpp);
see its section below.

---

### ConnectionHandler

**File:** [src/handlers/ConnectionHandler.hpp](../src/handlers/ConnectionHandler.hpp)

**Purpose:** Socket I/O and persistent connection management.

#### Constructor

```cpp
ConnectionHandler(std::shared_ptr<RouteHandler> routeHandler);
```

Creates connection handler with route handler.

**Parameters:**
- `routeHandler` - Shared pointer to RouteHandler

---

#### Methods

##### `void handleClient(int clientSocket)`

Main connection handling loop.

**Parameters:**
- `clientSocket` - Client socket file descriptor

**Process:**
1. Set 30s timeout
2. Loop while keep-alive:
   - Read request
   - Parse request
   - Route request
   - Send response
   - Check Connection header
3. Close socket

**Example:**
```cpp
connHandler->handleClient(client_fd);
```

**Persistent Connections:** Supports HTTP/1.1 keep-alive

---

##### `std::string readRequest(int socket)`

Reads HTTP request from socket.

**Parameters:**
- `socket` - Socket file descriptor

**Returns:** Raw HTTP request string

**Throws:** `std::runtime_error` on read error

**Logic:**
1. Read until `\r\n\r\n` (end of headers)
2. If `Content-Length` present, read body

---

##### `void sendResponse(int socket, const HttpResponse& response)`

Sends HTTP response to socket.

**Parameters:**
- `socket` - Socket file descriptor
- `response` - HTTP response to send

**Throws:** `std::runtime_error` on send error

**Example:**
```cpp
handler.sendResponse(client_fd, response);
```

---

## Server

### ThreadPool

**File:** [src/server/ThreadPool.hpp](../src/server/ThreadPool.hpp)

**Purpose:** Worker thread pool for concurrent request handling.

#### Constructor

```cpp
ThreadPool(size_t numThreads = std::thread::hardware_concurrency());
```

Creates thread pool with N workers.

**Parameters:**
- `numThreads` - Number of worker threads (default: CPU cores)

**Example:**
```cpp
ThreadPool pool(8);  // 8 workers
```

---

#### Destructor

```cpp
~ThreadPool();
```

Stops all threads and waits for completion.

**Behavior:**
1. Sets stop flag
2. Notifies all workers
3. Joins all threads

---

#### Methods

##### `void enqueue(std::function<void()> task)`

Adds task to queue.

**Parameters:**
- `task` - Function to execute

**Thread Safety:** ✅ Yes (mutex-protected)

**Example:**
```cpp
pool.enqueue([socket]() {
    handleClient(socket);
});
```

**Synchronization:**
1. Lock queue mutex
2. Push task
3. Unlock mutex
4. Notify one worker

---

### Server

**File:** [src/server/Server.hpp](../src/server/Server.hpp)

**Purpose:** Main HTTP server orchestrator.

#### Constructor

```cpp
Server(int port = 4221, const std::string& directory = "");
```

Creates server with port and file directory.

**Parameters:**
- `port` - Port to bind (default 4221)
- `directory` - Base directory for files (default: current)

**Example:**
```cpp
Server server(8080, "/var/www");
```

---

#### Methods

##### `void start()`

Starts server (blocking).

**Process:**
1. Create socket
2. Bind to port
3. Listen (backlog=128)
4. Accept loop:
   - Accept client
   - Enqueue to ThreadPool

**Example:**
```cpp
server.start();  // Blocks until stopped
```

**Signals:** Handles SIGINT/SIGTERM for graceful shutdown

---

##### `void stop()`

Stops server gracefully.

**Behavior:**
1. Stops accepting new connections
2. Waits for active connections to finish
3. Shuts down ThreadPool
4. Closes server socket

**Example:**
```cpp
// In signal handler
void signalHandler(int sig) {
    server.stop();
}
```

---

## Usage Examples

### Complete Request Flow

```cpp
// main.cpp
int main(int argc, char* argv[]) {
    std::string directory = parseDirectory(argc, argv);
    
    Server server(4221, directory);
    
    signal(SIGINT, signalHandler);
    
    Logger::info("Starting server on port 4221");
    server.start();
    
    return 0;
}

// Inside Server::start()
void Server::acceptLoop() {
    while (running_) {
        int client = accept(serverSocket_, ...);
        
        threadPool_->enqueue([this, client]() {
            connectionHandler_->handleClient(client);
        });
    }
}

// Inside ConnectionHandler::handleRequest(): the framing that makes
// pipelining work. readBuffer_ survives across calls, so one recv() that
// returns the end of request N and the start of request N+1 is fine.
ConnectionHandler::ReadResult ConnectionHandler::readRequest(std::string& out) {
    out.clear();
    for (;;) {
        const size_t headerEnd = readBuffer_.find("\r\n\r\n");
        if (headerEnd != std::string::npos) {
            const size_t bodyStart = headerEnd + 4;
            size_t contentLength = 0;
            bool hasBody = false;

            HttpRequest probe;
            if (probe.parse(readBuffer_)) {
                const std::string declared = probe.getHeader("Content-Length");
                if (!declared.empty()) {
                    // digits only: "abc", "-5" and out-of-range are all refused
                    if (!parseContentLength(declared, MAX_REQUEST_SIZE, contentLength)) {
                        return ReadResult::Invalid;   // -> 400
                    }
                    hasBody = true;
                }
            }

            const size_t needed = bodyStart + (hasBody ? contentLength : 0);
            if (readBuffer_.size() >= needed) {
                out = readBuffer_.substr(0, needed);   // exactly one request
                readBuffer_.erase(0, needed);          // surplus stays buffered
                return ReadResult::Ok;
            }
        }
        // ... recv() more bytes, or return Closed on EOF/timeout
    }
}

// Inside RouteHandler::handleRequest(): a dispatcher, nothing more.
HttpResponse RouteHandler::handleRequest(const HttpRequest& req) {
    return routes::dispatch(req, fileHandler_);
}

// Inside routes::dispatch(): resolution order, then compression once.
for (const Route& route : routeTable()) {
    if (!pathMatches(req.getPath(), route.pathPattern)) continue;
    pathMatchedSomeRoute = true;
    if (req.getMethod() != route.method) continue;

    HttpResponse response = route.handler(RouteContext{req, fileHandler_});
    applyContentEncoding(response, req);   // dispatcher owns this (Rule 8)
    return response;
}
return pathMatchedSomeRoute ? HttpResponse::methodNotAllowed(...)
                            : HttpResponse::notFound(...);

// The compression policy lives here, not in any route: whether a body may be
// compressed depends on the request and the response, not on the endpoint.
void applyContentEncoding(HttpResponse& res, const HttpRequest& req) {
    const ContentEncoding coding =
        EncodingNegotiator::negotiate(req.getHeader("Accept-Encoding"));

    if (coding == ContentEncoding::Identity) return;

    // The response's Content-Type decides, not the request's.
    std::string contentType = res.getHeader("Content-Type");
    if (contentType.empty()) contentType = MIME_TEXT_PLAIN;

    if (!ResponseEncoder::shouldCompress(res.getBody(), contentType)) return;

    const auto compressed = ResponseEncoder::compress(res.getBody(), coding);

    // An empty result means compression failed: fall back to identity rather
    // than failing the request.
    if (!compressed.empty()) {
        res.setCompressedBody(compressed, toHeaderValue(coding));
    }
}
```

---

**API Reference mukammal! Sab kuch documented!**

**(API Reference complete! Everything documented!)**
