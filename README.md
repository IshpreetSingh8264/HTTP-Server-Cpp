# 🍛 HTTP Dhaba Server - Pinglish Edition

[![progress-banner](https://backend.codecrafters.io/progress/http-server/445bc82a-2d71-49fc-afc5-a57e2f6e1f24)](https://app.codecrafters.io/users/codecrafters-bot?r=2qF)

> **Sat Sri Akaal!** A fully-featured HTTP/1.1 server built in C++23 with humor, love, and lots of Pinglish comments!

This is a complete solution to the ["Build Your Own HTTP server" Challenge](https://app.codecrafters.io/courses/http-server/overview) from CodeCrafters, implementing all stages with production-quality code, comprehensive documentation, and hilarious Punjabi-English comments!

## 🌟 Features

This HTTP server implements **all CodeCrafters stages** plus more:

### Core Features ✅
- ✅ **Bind to port** - Socket setup with SO_REUSEADDR
- ✅ **Respond with 200** - Basic HTTP responses
- ✅ **Extract URL path** - Request parsing
- ✅ **Respond with body** - Content-Length support
- ✅ **Read headers** - Complete header parsing
- ✅ **Concurrent connections** - Thread pool with `hardware_concurrency()`
- ✅ **Return files** - File serving with MIME type detection
- ✅ **Read request body** - POST request support
- ✅ **HTTP Compression** - Full gzip support with zlib

### Advanced Features 🚀
- ✅ **Compression headers** - Accept-Encoding parsing
- ✅ **Multiple compression schemes** - Gzip + deflate detection
- ✅ **Gzip compression** - Automatic compression for text content
- ✅ **Persistent connections** - HTTP/1.1 keep-alive
- ✅ **Concurrent persistent connections** - Multiple clients with keep-alive
- ✅ **Connection closure** - Graceful timeout and shutdown
- ✅ **Path traversal protection** - Secure file serving
- ✅ **Signal handling** - Ctrl+C graceful shutdown

## 🏗️ Architecture

```
┌─────────────┐
│   Client    │
└──────┬──────┘
       │ HTTP Request
       ▼
┌─────────────────────────────────────┐
│          Server (main.cpp)          │
│  ┌────────────────────────────────┐ │
│  │  Socket Bind (port 4221)       │ │
│  │  Signal Handlers (Ctrl+C)      │ │
│  └────────────┬───────────────────┘ │
│               │                      │
│               ▼                      │
│  ┌────────────────────────────────┐ │
│  │      Thread Pool               │ │
│  │  (hardware_concurrency threads)│ │
│  └────────────┬───────────────────┘ │
│               │                      │
│               ▼                      │
│  ┌────────────────────────────────┐ │
│  │   ConnectionHandler            │ │
│  │  • Read HTTP request           │ │
│  │  • Persistent connections      │ │
│  │  • 30s timeout                 │ │
│  └────────────┬───────────────────┘ │
│               │                      │
│               ▼                      │
│  ┌────────────────────────────────┐ │
│  │      RouteHandler              │ │
│  │  Routes:                       │ │
│  │   GET /                        │ │
│  │   GET /echo/{str}              │ │
│  │   GET /user-agent              │ │
│  │   GET /files/{name}            │ │
│  │   POST /files/{name}           │ │
│  └────┬───────────────────┬───────┘ │
│       │                   │          │
│       ▼                   ▼          │
│  ┌──────────┐      ┌──────────────┐ │
│  │  Gzip    │      │ FileHandler  │ │
│  │Compressor│      │ • Read/Write │ │
│  └──────────┘      └──────────────┘ │
└─────────────────────────────────────┘
```

## 📁 Project Structure

```
codecrafters-http-server-cpp/
├── docs/
│   ├── ARCHITECTURE.md      # System design & flow
│   ├── LEARNING_GUIDE.md    # Deep-dive concepts
│   └── API_REFERENCE.md     # Class/method docs
├── include/
│   ├── server/
│   │   ├── Server.hpp       # Main server orchestrator
│   │   └── ThreadPool.hpp   # Worker thread pool
│   ├── http/
│   │   ├── HttpRequest.hpp  # Request parser
│   │   ├── HttpResponse.hpp # Response builder
│   │   └── HttpConstants.hpp# HTTP constants
│   ├── handlers/
│   │   ├── ConnectionHandler.hpp  # Socket lifecycle
│   │   ├── RouteHandler.hpp       # URL routing
│   │   └── FileHandler.hpp        # File operations
│   ├── compression/
│   │   └── GzipCompressor.hpp     # Gzip compression
│   └── utils/
│       ├── Logger.hpp       # Pinglish logging
│       └── StringUtils.hpp  # String helpers
├── src/
│   ├── main.cpp             # Entry point
│   └── (mirrors include/)
├── CMakeLists.txt           # Build configuration
└── vcpkg.json               # Dependencies
```

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
# Run server on default port 4221
./build/http-server

# Run with file serving from specific directory
./build/http-server --directory /path/to/files
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

# Test gzip compression
curl -H "Accept-Encoding: gzip" http://localhost:4221/echo/hello --compressed

# Test file upload
curl -X POST http://localhost:4221/files/newfile.txt -d "File content here"

# Test persistent connections
curl -v http://localhost:4221/ http://localhost:4221/echo/test
```

## 🎯 Supported HTTP Routes

| Method | Path | Description | Example |
|--------|------|-------------|---------|
| GET | `/` | Root endpoint | `curl http://localhost:4221/` |
| GET | `/echo/{str}` | Echo back string | `curl http://localhost:4221/echo/hello` |
| GET | `/user-agent` | Return User-Agent header | `curl http://localhost:4221/user-agent` |
| GET | `/files/{name}` | Serve file from directory | `curl http://localhost:4221/files/test.txt` |
| POST | `/files/{name}` | Save file to directory | `curl -X POST http://localhost:4221/files/new.txt -d "content"` |

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

The server passes **all CodeCrafters stages**:

```sh
git add .
git commit -m "Complete HTTP server implementation"
git push origin master
```

All stages will pass:
1. ✅ Bind to port
2. ✅ Respond with 200
3. ✅ Extract URL path
4. ✅ Respond with body
5. ✅ Read headers
6. ✅ Concurrent connections
7. ✅ Return a file
8. ✅ Read request body
9. ✅ Compression headers
10. ✅ Multiple compression schemes
11. ✅ Gzip compression
12. ✅ Persistent connections

## 🔧 Configuration

### Thread Pool Size

By default, the server uses `std::thread::hardware_concurrency()` worker threads. Modify in [Server.hpp](include/server/Server.hpp):

```cpp
// Use specific number of threads
threadPool_ = std::make_unique<ThreadPool>(8);
```

### Connection Timeout

Default: **30 seconds** (HTTP/1.1 standard). Modify in [ConnectionHandler.hpp](include/handlers/ConnectionHandler.hpp):

```cpp
static constexpr int TIMEOUT_SECONDS = 30;
```

### Port Number

Default: **4221**. Modify in [main.cpp](src/main.cpp):

```cpp
server::Server httpServer(8080, fileDirectory);  // Use port 8080
```

## 🛡️ Security Features

- **Path Traversal Protection** - Prevents `../` attacks
- **Request Size Limits** - Max 1MB request size
- **Timeout Protection** - 30s read timeout
- **Safe File Operations** - Validates all file paths

## 🤝 Contributing

This is a learning project with Pinglish humor! Feel free to:
- Add more Pinglish comments
- Improve error messages
- Add new features
- Fix bugs

## 📝 License

This project is for educational purposes (CodeCrafters challenge).

## 🙏 Acknowledgments

- **CodeCrafters** - For the awesome challenge
- **Punjabi culture** - For the humor and warmth
- **C++ community** - For amazing tools and libraries

---

**Sat Sri Akaal! Happy coding! 🚀**
