#pragma once

// ConnectionHandler.hpp - Client connection management
// Socket connection nu handle karo - read, write, timeout, persistence!
// (Handle socket connection - read, write, timeout, persistence!)

#include <memory>
#include <string>

#include "handlers/RouteHandler.hpp"
#include "http/HttpRequest.hpp"
#include "http/HttpResponse.hpp"

namespace handlers {

/**
 * ConnectionHandler - Own one client socket for its whole lifetime
 *
 * Purpose: Socket lifecycle manage karo - accept te close tak
 *          (Manage the socket lifecycle - from accept to close)
 *
 * This is the only layer that knows about file descriptors and keep-alive
 * framing. It hands a parsed HttpRequest down to RouteHandler and writes the
 * HttpResponse back. Everything cross-cutting about the connection (timeout,
 * keep-alive, body framing) is owned here, not in the route handlers.
 */
class ConnectionHandler {
private:
    int clientSocket_;                           // Client socket file descriptor
    std::shared_ptr<RouteHandler> routeHandler_; // Request routing lai

    static constexpr int BUFFER_SIZE = 8192;     // 8KB read buffer
    static constexpr int TIMEOUT_SECONDS = 30;   // 30 second read timeout
    static constexpr size_t MAX_REQUEST_SIZE = 1024 * 1024;  // 1MB cap

    void setSocketTimeout();
    std::string readRequest();
    bool sendResponse(const http::HttpResponse& response);
    void closeConnection();

public:
    ConnectionHandler(int clientSocket, std::shared_ptr<RouteHandler> routeHandler);

    ~ConnectionHandler();

    ConnectionHandler(const ConnectionHandler&) = delete;
    ConnectionHandler& operator=(const ConnectionHandler&) = delete;

    /// Serve requests on this socket until close, timeout, or peer disconnect.
    void handleConnection();
};

} // namespace handlers
