#pragma once

// Server.hpp - Main HTTP server orchestrator
// Saari cheezaan nu ikatha karke server chalao!
// (Bring everything together and run the server!)

#include <atomic>
#include <memory>
#include <string>

#include "handlers/FileHandler.hpp"
#include "handlers/RouteHandler.hpp"
#include "server/ThreadPool.hpp"

namespace server {

/**
 * Server - Owns the listening socket and the component graph
 *
 * Purpose: Complete HTTP server - socket setup, connection handling, routing
 *          (Complete HTTP server - socket setup, connection handling, routing)
 *
 * Lifecycle:
 *   1. Constructor wires FileHandler -> RouteHandler -> ThreadPool
 *   2. start() creates/binds/listens on the socket (blocking accept loop)
 *   3. each accepted socket becomes one ThreadPool task
 *   4. ConnectionHandler -> RouteHandler -> FileHandler serves the client
 *
 * The only layer that knows about bind/listen/accept.
 */
class Server {
private:
    int serverSocket_;                             // Server socket FD
    int port_;                                     // Server port
    std::string fileDirectory_;                    // Base directory for files

    std::unique_ptr<ThreadPool> threadPool_;
    std::shared_ptr<handlers::FileHandler> fileHandler_;
    std::shared_ptr<handlers::RouteHandler> routeHandler_;

    std::atomic<bool> running_;

    static constexpr int BACKLOG = 128;  // Connection backlog

    bool createSocket();
    bool setSocketOptions();
    bool bindSocket();
    bool listenForConnections();
    void acceptLoop();
    void handleClient(int clientSocket);

public:
    Server(int port = 4221, const std::string& fileDir = ".");
    ~Server();

    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    /// Create, bind and serve. Blocks until stop() is called.
    void start();

    /// Close the listening socket. Safe to call from a signal handler or twice.
    void stop();

    int getPort() const { return port_; }
};

} // namespace server
