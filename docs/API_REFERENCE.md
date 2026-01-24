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
   - [GzipCompressor](#gzipcompressor)
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

**File:** [include/utils/Logger.hpp](../include/utils/Logger.hpp)

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

**File:** [include/utils/StringUtils.hpp](../include/utils/StringUtils.hpp)

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

**File:** [include/http/HttpConstants.hpp](../include/http/HttpConstants.hpp)

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

**File:** [include/http/HttpRequest.hpp](../include/http/HttpRequest.hpp)

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

**File:** [include/http/HttpResponse.hpp](../include/http/HttpResponse.hpp)

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

##### `void setCompressedBody(const std::string& compressed, const std::string& encoding)`

Sets compressed response body.

**Parameters:**
- `compressed` - Compressed body data
- `encoding` - Encoding type (e.g., "gzip")

**Example:**
```cpp
auto compressed = GzipCompressor::compress(body);
response.setCompressedBody(compressed, "gzip");
```

**Note:** Sets both Content-Length and Content-Encoding headers.

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

### GzipCompressor

**File:** [include/compression/GzipCompressor.hpp](../include/compression/GzipCompressor.hpp)

**Purpose:** Gzip compression using zlib.

#### Methods

##### `static std::string compress(const std::string& data)`

Compresses data using gzip.

**Parameters:**
- `data` - Uncompressed data

**Returns:** Gzip-compressed data

**Throws:** `std::runtime_error` if compression fails

**Example:**
```cpp
std::string original = "Hello World! " * 100;  // Large text
std::string compressed = GzipCompressor::compress(original);
```

**Algorithm:** DEFLATE with gzip wrapper (zlib windowBits=15|16)

---

##### `static bool supportsGzip(const std::string& acceptEncoding)`

Checks if client supports gzip.

**Parameters:**
- `acceptEncoding` - Accept-Encoding header value

**Returns:** `true` if "gzip" found, `false` otherwise

**Example:**
```cpp
std::string ae = req.getHeader("Accept-Encoding");
if (GzipCompressor::supportsGzip(ae)) {
    // Compress response
}
```

---

##### `static bool shouldCompress(const std::string& contentType, size_t dataSize)`

Determines if compression is beneficial.

**Parameters:**
- `contentType` - Content-Type header value
- `dataSize` - Data size in bytes

**Returns:** `true` if should compress, `false` otherwise

**Logic:**
- Data size must be > 1024 bytes (1KB)
- Content-Type must start with "text/" or be "application/json"

**Example:**
```cpp
if (GzipCompressor::shouldCompress("text/html", body.size())) {
    auto compressed = GzipCompressor::compress(body);
}
```

---

## Handlers

### FileHandler

**File:** [include/handlers/FileHandler.hpp](../include/handlers/FileHandler.hpp)

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

**File:** [include/handlers/RouteHandler.hpp](../include/handlers/RouteHandler.hpp)

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

##### `HttpResponse handleGetRequest(const HttpRequest& request)`

Handles GET requests.

**Parameters:**
- `request` - HTTP request

**Returns:** HttpResponse

**Internal Routing:**
```cpp
if (path == "/") return handleRoot();
if (startsWith(path, "/echo/")) return handleEcho(request);
if (path == "/user-agent") return handleUserAgent(request);
if (startsWith(path, "/files/")) return handleFileGet(request);
```

---

##### `HttpResponse handlePostRequest(const HttpRequest& request)`

Handles POST requests.

**Parameters:**
- `request` - HTTP request

**Returns:** HttpResponse

**Routing:** Only `/files/{name}` supported for POST

---

### ConnectionHandler

**File:** [include/handlers/ConnectionHandler.hpp](../include/handlers/ConnectionHandler.hpp)

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

**File:** [include/server/ThreadPool.hpp](../include/server/ThreadPool.hpp)

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

**File:** [include/server/Server.hpp](../include/server/Server.hpp)

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

// Inside ConnectionHandler::handleClient()
void ConnectionHandler::handleClient(int socket) {
    setSocketTimeout(socket, 30);
    
    bool keepAlive = true;
    while (keepAlive) {
        std::string rawReq = readRequest(socket);
        
        HttpRequest req;
        req.parse(rawReq);
        
        HttpResponse res = routeHandler_->handleRequest(req);
        
        if (req.getHeader("Connection") == "close") {
            keepAlive = false;
            res.setHeader("Connection", "close");
        }
        
        sendResponse(socket, res);
    }
    
    close(socket);
}

// Inside RouteHandler::handleRequest()
HttpResponse RouteHandler::handleRequest(const HttpRequest& req) {
    std::string path = req.getPath();
    
    if (path == "/") {
        return HttpResponse::ok();
    }
    
    if (StringUtils::startsWith(path, "/echo/")) {
        std::string str = path.substr(6);
        HttpResponse res = HttpResponse::ok(str);
        applyCompression(res, req);
        return res;
    }
    
    return HttpResponse::notFound();
}

// Compression helper
void RouteHandler::applyCompression(HttpResponse& res, const HttpRequest& req) {
    std::string ae = req.getHeader("Accept-Encoding");
    
    if (!GzipCompressor::supportsGzip(ae)) return;
    
    std::string body = res.getBody();
    std::string contentType = res.getHeader("Content-Type");
    
    if (!GzipCompressor::shouldCompress(contentType, body.size())) return;
    
    std::string compressed = GzipCompressor::compress(body);
    res.setCompressedBody(compressed, "gzip");
    
    Logger::info("Compressed: " + std::to_string(body.size()) + " → " + 
                 std::to_string(compressed.size()) + " bytes");
}
```

---

**API Reference mukammal! Sab kuch documented!**

**(API Reference complete! Everything documented!)**
