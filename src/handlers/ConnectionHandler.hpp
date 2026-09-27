#pragma once

// ConnectionHandler.hpp - Client connection management
// Socket connection nu handle karo - read, write, timeout, persistence!
// (Handle socket connection - read, write, timeout, persistence!)

#include <cstddef>
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
 * This is the only layer that knows about file descriptors and request
 * framing. It hands a parsed HttpRequest down to RouteHandler and writes the
 * HttpResponse back. Everything cross-cutting about the connection (timeout,
 * keep-alive, body length) is owned here, not in the route handlers.
 *
 * Framing note: recv() has no idea where a request ends, so one recv() may
 * return the tail of request N and the head of request N+1. The handler
 * therefore keeps a read buffer across calls and hands out exactly one
 * request at a time; the surplus stays buffered for the next iteration. That
 * is what makes HTTP pipelining work.
 */
class ConnectionHandler {
public:
    /// Outcome of trying to read one request off the socket.
    enum class ReadResult {
        Ok,       ///< outRequest holds exactly one complete request
        Closed,   ///< peer closed, timed out, or the request was oversized
        Invalid   ///< framing is malformed; answer 400 and close
    };

private:
    int clientSocket_;                           // Client socket file descriptor
    std::shared_ptr<RouteHandler> routeHandler_; // Request routing lai
    std::string readBuffer_;                     // Read but not yet consumed

    static constexpr int BUFFER_SIZE = 8192;     // 8KB read buffer
    static constexpr int TIMEOUT_SECONDS = 30;   // 30 second read timeout
    static constexpr size_t MAX_REQUEST_SIZE = 1024 * 1024;  // 1MB cap

    void setSocketTimeout();
    ReadResult readRequest(std::string& outRequest);
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
