# 📚 HTTP Server Learning Guide

> **Iss guide vich sab kuch seekho - networking se lekar compression tak!**
> 
> (Learn everything in this guide - from networking to compression!)

This comprehensive guide explains the concepts, technologies, and design patterns used in building a production-quality HTTP server. Each section includes theory, code snippets, and references to the actual implementation.

---

## Table of Contents

1. [TCP/IP & Socket Programming](#1-tcpip--socket-programming)
2. [HTTP Protocol Internals](#2-http-protocol-internals)
3. [Concurrency & Threading](#3-concurrency--threading)
4. [Compression Algorithms](#4-compression-algorithms)
5. [Persistent Connections](#5-persistent-connections)
6. [Security Considerations](#6-security-considerations)

---

## 1. TCP/IP & Socket Programming

### What is a Socket?

**Simple Explanation:**
> Socket ik endpoint hai network communication lai - jivein phone da number phone calls lai
> 
> (A socket is an endpoint for network communication - like a phone number for phone calls)

A socket is a combination of:
- **IP Address** - Where? (e.g., 192.168.1.1)
- **Port Number** - Which service? (e.g., 4221)
- **Protocol** - How? (TCP or UDP)

### Socket Lifecycle

```
┌─────────────┐
│   socket()  │  Create socket file descriptor
└──────┬──────┘
       │
       ▼
┌─────────────┐
│    bind()   │  Bind to IP:Port
└──────┬──────┘
       │
       ▼
┌─────────────┐
│   listen()  │  Mark as passive socket (server)
└──────┬──────┘
       │
       ▼
┌─────────────┐
│   accept()  │  Wait for & accept client connections (blocking)
└──────┬──────┘
       │
       ▼
┌─────────────┐
│ recv/send   │  Read/write data
└──────┬──────┘
       │
       ▼
┌─────────────┐
│   close()   │  Close connection
└─────────────┘
```

### Creating a Socket

**Code Snippet:**
```cpp
// Socket banao - TCP connection lai
// (Create socket - for TCP connection)
int server_fd = socket(AF_INET, SOCK_STREAM, 0);
if (server_fd < 0) {
    // Error handling
    perror("socket");
    return -1;
}
```

**Parameters:**
- `AF_INET` - IPv4 address family
- `SOCK_STREAM` - TCP (reliable, connection-oriented)
- `0` - Protocol (0 = auto-select TCP for SOCK_STREAM)

**Implementation:** See [Server.cpp:createSocket()](../include/server/Server.hpp#L139)

### Socket Options - SO_REUSEADDR

**Why needed?**
> Jado server restart hunda hai, toh port "TIME_WAIT" state vich hunda hai
> 
> (When server restarts, the port is in "TIME_WAIT" state)

Without `SO_REUSEADDR`, you get: `Address already in use`

**Code Snippet:**
```cpp
// Address reuse kar sakde haan - testing lai zaroori!
// (Can reuse address - essential for testing!)
int reuse = 1;
if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
    perror("setsockopt");
}
```

**Implementation:** See [Server.cpp:setSocketOptions()](../include/server/Server.hpp#L156)

### Binding to Address

**Code Snippet:**
```cpp
struct sockaddr_in server_addr;
server_addr.sin_family = AF_INET;           // IPv4
server_addr.sin_addr.s_addr = INADDR_ANY;   // All interfaces (0.0.0.0)
server_addr.sin_port = htons(4221);         // Port 4221 (network byte order!)

if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) != 0) {
    perror("bind");
    return -1;
}
```

**Key Concept: Network Byte Order**
> Network byte order = Big-Endian (most significant byte first)
> Host byte order = varies by CPU (Intel = Little-Endian)
> 
> Use `htons()` (host to network short) to convert!

**Implementation:** See [Server.cpp:bindSocket()](../include/server/Server.hpp#L171)

### Listening for Connections

**Code Snippet:**
```cpp
int backlog = 128;  // Queue size for pending connections
if (listen(server_fd, backlog) != 0) {
    perror("listen");
    return -1;
}
```

**What is backlog?**
> Kitne pending connections queue vich rakh sakde - agar server busy hai
> 
> (How many pending connections can be queued - if server is busy)

**Implementation:** See [Server.cpp:listenForConnections()](../include/server/Server.hpp#L190)

### Accepting Connections

**Code Snippet:**
```cpp
struct sockaddr_in client_addr;
socklen_t client_len = sizeof(client_addr);

int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
if (client_fd < 0) {
    perror("accept");
    return -1;
}

// Client IP address pata karo
// (Get client IP address)
char client_ip[INET_ADDRSTRLEN];
inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
printf("Client connected: %s\n", client_ip);
```

**Blocking vs Non-blocking:**
- `accept()` is **blocking** - waits until client connects
- For concurrent servers, run in loop and dispatch to threads

**Implementation:** See [Server.cpp:acceptLoop()](../include/server/Server.hpp#L203)

### Reading & Writing Data

**Reading:**
```cpp
char buffer[8192];
ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

if (bytes_read < 0) {
    perror("recv");  // Error
} else if (bytes_read == 0) {
    // Client disconnected
} else {
    buffer[bytes_read] = '\0';  // Null-terminate
    // Process data...
}
```

**Writing:**
```cpp
const char* response = "HTTP/1.1 200 OK\r\n\r\n";
ssize_t bytes_sent = send(client_fd, response, strlen(response), 0);

if (bytes_sent < 0) {
    perror("send");
}
```

**Implementation:** See [ConnectionHandler.cpp](../include/handlers/ConnectionHandler.hpp)

---

## 2. HTTP Protocol Internals

### HTTP Request Format

```
GET /echo/hello HTTP/1.1\r\n          ← Request Line
Host: localhost:4221\r\n               ← Headers
User-Agent: curl/7.81.0\r\n
Accept-Encoding: gzip\r\n
\r\n                                   ← Empty line (end of headers)
[Optional Body]                        ← Body (for POST/PUT)
```

**Components:**
1. **Request Line:** `METHOD PATH VERSION`
2. **Headers:** `Name: Value` pairs
3. **Empty Line:** `\r\n\r\n` separates headers from body
4. **Body:** Optional (POST/PUT)

### Parsing HTTP Requests

**Algorithm:**
```
1. Read data until "\r\n\r\n" found (header separator)
2. Split by "\r\n" to get lines
3. First line = Request Line (split by space)
4. Remaining lines = Headers (split by ":")
5. If Content-Length header exists:
   - Read Content-Length bytes for body
```

**Code Snippet:**
```cpp
// Request line parse karo: "GET /path HTTP/1.1"
// (Parse request line)
auto parts = split(line, ' ');
if (parts.size() != 3) {
    return false;  // Invalid format
}

method_ = parts[0];   // GET, POST, etc.
path_ = parts[1];     // /echo/hello
version_ = parts[2];  // HTTP/1.1
```

**Implementation:** See [HttpRequest.cpp:parse()](../include/http/HttpRequest.hpp#L39)

### HTTP Response Format

```
HTTP/1.1 200 OK\r\n                    ← Status Line
Content-Type: text/plain\r\n           ← Headers
Content-Length: 5\r\n
Content-Encoding: gzip\r\n
Connection: keep-alive\r\n
\r\n                                   ← Empty line
Hello                                  ← Body (5 bytes)
```

**Building Responses:**
```cpp
std::ostringstream response;

// Status line
response << "HTTP/1.1 200 OK\r\n";

// Headers
response << "Content-Type: text/plain\r\n";
response << "Content-Length: " << body.size() << "\r\n";
response << "\r\n";

// Body
response << body;
```

**Implementation:** See [HttpResponse.cpp:toString()](../include/http/HttpResponse.hpp#L115)

### HTTP Status Codes

| Code | Meaning | Use Case |
|------|---------|----------|
| 200 | OK | Success |
| 201 | Created | Resource created (POST) |
| 400 | Bad Request | Invalid request format |
| 404 | Not Found | Resource doesn't exist |
| 500 | Internal Server Error | Server crashed |

**Implementation:** See [HttpConstants.hpp](../include/http/HttpConstants.hpp)

### MIME Types

> MIME type batanda hai ki data koi format vich hai
> 
> (MIME type tells what format the data is in)

Common types:
- `text/plain` - Plain text
- `text/html` - HTML document
- `application/json` - JSON data
- `application/octet-stream` - Binary data
- `image/png` - PNG image

**Auto-detection from file extension:**
```cpp
std::string getMimeType(const std::string& filename) {
    if (endsWith(filename, ".html")) return "text/html";
    if (endsWith(filename, ".jpg")) return "image/jpeg";
    // ... more types
    return "application/octet-stream";  // Default
}
```

**Implementation:** See [HttpConstants.cpp:getMimeType()](../include/http/HttpConstants.hpp#L129)

---

## 3. Concurrency & Threading

### Why Concurrency?

**Problem:**
> Agar single-threaded server hai, toh ekk client request handle kar reha te dusra wait kar reha
> 
> (If single-threaded server, one client is being served while others wait)

**Solution:**
> Thread pool - multiple workers handle requests simultaneously
> 
> (Thread pool - multiple workers handle requests at same time)

### Thread-Per-Connection vs Thread Pool

**Thread-Per-Connection:**
```cpp
while (true) {
    int client = accept(server_fd, ...);
    std::thread([client]() {
        handle_client(client);
    }).detach();  // Start thread and forget
}
```

**Disadvantages:**
- Thread creation expensive (memory, context switching)
- Unlimited threads = resource exhaustion
- No control over concurrency

**Thread Pool (Better!):**
```cpp
ThreadPool pool(8);  // 8 worker threads

while (true) {
    int client = accept(server_fd, ...);
    pool.enqueue([client]() {
        handle_client(client);
    });
}
```

**Advantages:**
- Reuse threads (no creation overhead)
- Limit concurrency (resource control)
- Better performance

### Thread Pool Implementation

**Architecture:**
```
┌──────────────────────────────────┐
│        Task Queue                │
│  ┌────┐ ┌────┐ ┌────┐ ┌────┐    │
│  │ T1 │ │ T2 │ │ T3 │ │ T4 │    │
│  └────┘ └────┘ └────┘ └────┘    │
└───────────┬──────────────────────┘
            │
   ┌────────┴────────┐
   │                 │
   ▼                 ▼
┌────────┐        ┌────────┐
│Worker 1│        │Worker 2│  ... Worker N
└────────┘        └────────┘
```

**Key Components:**
1. **Task Queue:** `std::queue<std::function<void()>>`
2. **Worker Threads:** `std::vector<std::thread>`
3. **Mutex:** Protect queue access
4. **Condition Variable:** Wake up workers

**Code Snippet:**
```cpp
class ThreadPool {
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex queue_mutex_;
    std::condition_variable condition_;
    bool stop_;

public:
    ThreadPool(size_t num_threads) : stop_(false) {
        for (size_t i = 0; i < num_threads; ++i) {
            workers_.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex_);
                        condition_.wait(lock, [this] {
                            return stop_ || !tasks_.empty();
                        });
                        
                        if (stop_ && tasks_.empty()) return;
                        
                        task = std::move(tasks_.front());
                        tasks_.pop();
                    }
                    
                    task();  // Execute outside lock!
                }
            });
        }
    }
};
```

**Implementation:** See [ThreadPool.hpp](../include/server/ThreadPool.hpp)

### Synchronization Primitives

**Mutex (Mutual Exclusion):**
> Critical section nu protect karda - ekk time te ekk thread hi access kar sakda
> 
> (Protects critical section - only one thread can access at a time)

```cpp
std::mutex mtx;

void thread_safe_operation() {
    std::lock_guard<std::mutex> lock(mtx);  // Auto lock/unlock
    // Critical section - only one thread here
}
```

**Condition Variable:**
> Thread nu sleep kar sakde te wake up kar sakde jado condition meet ho jave
> 
> (Can make thread sleep and wake up when condition is met)

```cpp
std::condition_variable cv;
std::mutex mtx;
bool ready = false;

// Wait until ready
std::unique_lock<std::mutex> lock(mtx);
cv.wait(lock, []{ return ready; });

// Signal ready
{
    std::lock_guard<std::mutex> lock(mtx);
    ready = true;
}
cv.notify_one();  // Wake up one waiting thread
```

**Atomic Variables:**
> Lock-free operations for simple types
> 
> (Lock-free - faster than mutex for simple variables)

```cpp
std::atomic<bool> running_{true};

// Thread-safe read/write
if (running_) { ... }
running_ = false;
```

### Hardware Concurrency

```cpp
unsigned int num_threads = std::thread::hardware_concurrency();
// Returns: Number of CPU cores/threads (e.g., 8 for 4-core CPU with hyperthreading)
```

**Why use it?**
> CPU cores de hisaab se optimal threads - na jyada na ghatt
> 
> (Optimal threads based on CPU cores - not too many, not too few)

**Implementation:** See [ThreadPool.hpp:constructor](../include/server/ThreadPool.hpp#L56)

---

## 4. Compression Algorithms

### Why Compression?

**Problem:**
> HTTP response vich text data (HTML, JSON, CSS) bohot vada ho sakda
> 
> (HTTP response with text data can be very large)

**Solution:**
> Gzip compression - data nu 60-80% tak chota kar sakde!
> 
> (Gzip compression - can reduce data by 60-80%!)

**Benefits:**
- Less bandwidth usage
- Faster downloads
- Lower server costs

### How Gzip Works

**Algorithm: DEFLATE (LZ77 + Huffman)**

1. **LZ77:** Find repeated patterns
   ```
   "hello hello hello" → "hello" + "repeat 2 times"
   ```

2. **Huffman Coding:** Frequent characters = shorter codes
   ```
   'e' (frequent) → 10 (2 bits)
   'z' (rare)     → 11010101 (8 bits)
   ```

### Gzip with zlib

**Code Snippet:**
```cpp
#include <zlib.h>

std::vector<char> compress_gzip(const std::string& data) {
    z_stream stream;
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;
    
    // windowBits = 15 + 16 means gzip format
    // 15 = default, +16 = add gzip wrapper
    deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 
                 15 + 16, 8, Z_DEFAULT_STRATEGY);
    
    stream.avail_in = data.size();
    stream.next_in = (Bytef*)data.data();
    
    std::vector<char> compressed(data.size() + 1024);
    stream.avail_out = compressed.size();
    stream.next_out = (Bytef*)compressed.data();
    
    deflate(&stream, Z_FINISH);
    
    compressed.resize(stream.total_out);
    deflateEnd(&stream);
    
    return compressed;
}
```

**Implementation:** See [GzipCompressor.cpp:compress()](../include/compression/GzipCompressor.hpp#L37)

### Content Negotiation

**Client Request:**
```
GET /data HTTP/1.1
Accept-Encoding: gzip, deflate, br
```

**Server checks:**
1. Parse `Accept-Encoding` header
2. Check if server supports gzip
3. Compress response
4. Add `Content-Encoding: gzip` header

**Code Snippet:**
```cpp
std::string accept_encoding = request.getHeader("Accept-Encoding");

if (contains(toLower(accept_encoding), "gzip")) {
    auto compressed = GzipCompressor::compress(body);
    response.setCompressedBody(compressed, "gzip");
}
```

**Implementation:** See [RouteHandler.cpp:applyCompression()](../include/handlers/RouteHandler.hpp#L308)

### When NOT to Compress

**Don't compress:**
- Small files (< 1KB) - overhead > benefit
- Already compressed (images, videos, zip files)

```cpp
bool should_compress(const std::string& data, const std::string& content_type) {
    if (data.size() < 1024) return false;  // Too small
    
    if (contains(content_type, "image/")) return false;  // Already compressed
    if (contains(content_type, "video/")) return false;
    
    return true;  // Text files benefit from compression
}
```

**Implementation:** See [GzipCompressor.cpp:shouldCompress()](../include/compression/GzipCompressor.hpp#L115)

---

## 5. Persistent Connections

### What is Connection Persistence?

**HTTP/1.0 (Old):**
```
1. Open TCP connection
2. Send request
3. Receive response
4. Close TCP connection
   ↓
Repeat for EVERY request!
```

**HTTP/1.1 (Modern):**
```
1. Open TCP connection
2. Send request 1 → Response 1
3. Send request 2 → Response 2
4. Send request 3 → Response 3
5. Close TCP connection (after timeout or explicit close)
```

**Benefits:**
- Fewer TCP handshakes (expensive!)
- Lower latency
- Better throughput

### Connection Header

**Keep-Alive:**
```
GET / HTTP/1.1
Connection: keep-alive
```

**Close:**
```
GET / HTTP/1.1
Connection: close
```

**Default:**
- HTTP/1.0: close
- HTTP/1.1: keep-alive

### Implementation Pattern

**Server Loop:**
```cpp
void handleConnection(int client_socket) {
    bool keep_alive = true;
    
    while (keep_alive) {
        // Read request
        std::string request = read_request(client_socket);
        
        if (request.empty()) break;  // Timeout or error
        
        // Parse & handle
        HttpRequest req;
        req.parse(request);
        
        HttpResponse res = handle_request(req);
        
        // Check Connection header
        std::string conn = req.getHeader("Connection");
        if (equalsIgnoreCase(conn, "close")) {
            keep_alive = false;
            res.setConnection("close");
        } else {
            res.setConnection("keep-alive");
        }
        
        // Send response
        send_response(client_socket, res);
    }
    
    close(client_socket);
}
```

**Implementation:** See [ConnectionHandler.cpp:handleConnection()](../include/handlers/ConnectionHandler.hpp#L53)

### Timeout Handling

**Why timeout?**
> Agar client disconnect ho jave bina "Connection: close" bhejeya, server forever wait nahi kar sakda
> 
> (If client disconnects without sending "Connection: close", server can't wait forever)

**Setting timeout:**
```cpp
struct timeval timeout;
timeout.tv_sec = 30;   // 30 seconds
timeout.tv_usec = 0;

setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
```

Now `recv()` will return error after 30 seconds of no data.

**Implementation:** See [ConnectionHandler.cpp:setSocketTimeout()](../include/handlers/ConnectionHandler.hpp#L109)

---

## 6. Security Considerations

### Path Traversal Attacks

**Attack Example:**
```
GET /files/../../etc/passwd HTTP/1.1
```

Attacker tries to access files outside allowed directory!

**Protection:**
```cpp
bool is_path_safe(const std::filesystem::path& requested_path) {
    auto canonical_base = std::filesystem::canonical(base_directory);
    auto canonical_path = std::filesystem::weakly_canonical(requested_path);
    
    // Check if requested path starts with base directory
    auto [base_end, path_end] = std::mismatch(
        canonical_base.begin(), canonical_base.end(),
        canonical_path.begin(), canonical_path.end()
    );
    
    return (base_end == canonical_base.end());  // Path is inside base
}
```

**Implementation:** See [FileHandler.cpp:isPathSafe()](../include/handlers/FileHandler.hpp#L235)

### Request Size Limits

**Attack:** Send infinite request to consume server memory

**Protection:**
```cpp
const size_t MAX_REQUEST_SIZE = 1024 * 1024;  // 1 MB

if (request.size() > MAX_REQUEST_SIZE) {
    return HttpResponse::badRequest("Request too large");
}
```

**Implementation:** See [ConnectionHandler.cpp:readRequest()](../include/handlers/ConnectionHandler.hpp#L134)

### Timeout Protection

**Attack:** Open connection and never send data (slowloris attack)

**Protection:**
- Set `SO_RCVTIMEO` socket option (30 seconds)
- Close connection if no data received

**Implementation:** See [ConnectionHandler.cpp:setSocketTimeout()](../include/handlers/ConnectionHandler.hpp#L109)

---

## Summary

**Key Takeaways:**

1. **Sockets** - Networking endpoints, TCP provides reliable communication
2. **HTTP** - Text-based protocol, requests/responses have specific format
3. **Threading** - Thread pool > thread-per-connection for scalability
4. **Compression** - Gzip reduces bandwidth, use for text content
5. **Persistence** - Keep-alive connections reduce latency
6. **Security** - Validate paths, limit sizes, set timeouts

**Complete Implementation:** See [Server.hpp](../include/server/Server.hpp) - ties everything together!

---

**Shabash! Tusi sab kuch sikh litta!**

**(Well done! You've learned everything!)**
