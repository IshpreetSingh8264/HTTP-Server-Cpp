#pragma once

// Server.hpp - Main HTTP server orchestrator
// Saari cheezaan nu ikatha karke server chalao!
// (Bring everything together and run the server!)

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <csignal>
#include <atomic>
#include <memory>
#include <string>
#include "server/ThreadPool.hpp"
#include "handlers/ConnectionHandler.hpp"
#include "handlers/RouteHandler.hpp"
#include "handlers/FileHandler.hpp"
#include "utils/Logger.hpp"

namespace server {

/**
 * Server - Main HTTP server class
 * 
 * Purpose: Complete HTTP server - socket setup, connection handling, routing
 *          (Complete HTTP server - socket setup, connection handling, routing)
 * 
 * Architecture:
 * 1. Create socket and bind to port 4221
 * 2. Start thread pool
 * 3. Accept connections in loop
 * 4. Dispatch each connection to thread pool
 * 5. ConnectionHandler handles client
 * 6. RouteHandler routes requests
 * 7. FileHandler serves files
 */
class Server {
private:
    int serverSocket_;                              // Server socket FD
    int port_;                                      // Server port
    std::string fileDirectory_;                     // Base directory for files
    
    std::unique_ptr<ThreadPool> threadPool_;                // Worker thread pool
    std::shared_ptr<handlers::FileHandler> fileHandler_;     // File operations
    std::shared_ptr<handlers::RouteHandler> routeHandler_;   // Request routing
    
    std::atomic<bool> running_;                     // Server running flag
    
    static constexpr int BACKLOG = 128;             // Connection backlog
                                                   // (Maximum pending connections)

public:
    /**
     * Constructor - Server setup karo
     * (Constructor - Setup server)
     * 
     * @param port: Port number (default 4221)
     * @param fileDir: Base directory for file serving (default ".")
     */
    Server(int port = 4221, const std::string& fileDir = ".") 
        : serverSocket_(-1), 
          port_(port), 
          fileDirectory_(fileDir),
          running_(false) {
        
        utils::Logger::info("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        utils::Logger::info("🍛 HTTP Dhaba Server - Pinglish Edition! 🍛");
        utils::Logger::info("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        // (HTTP Restaurant Server - Pinglish Edition!)
        
        utils::Logger::info("Server setup shuru kar rahe haan...");
        // (Starting server setup...)
        
        // File handler setup
        fileHandler_ = std::make_shared<handlers::FileHandler>(fileDirectory_);
        
        // Route handler setup
        routeHandler_ = std::make_shared<handlers::RouteHandler>(fileHandler_);
        
        // Thread pool setup - hardware_concurrency() workers
        threadPool_ = std::make_unique<ThreadPool>();
        
        utils::Logger::info("Server components tayar hain! Port: " + std::to_string(port_));
        // (Server components ready!)
    }

    /**
     * Destructor - Cleanup
     */
    ~Server() {
        stop();
    }

    // No copy/move
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    /**
     * Server start karo!
     * (Start the server!)
     * 
     * Blocks until server stops
     */
    void start() {
        utils::Logger::info("Server chaalu kar rahe haan port " + std::to_string(port_) + " te...");
        // (Starting server on port...)

        // Socket create karo
        // (Create socket)
        if (!createSocket()) {
            utils::Logger::error("Oye! Socket ni baneya, server start nahi ho sakda!");
            // (Hey! Socket didn't create, server can't start!)
            return;
        }

        // Socket options set karo
        // (Set socket options)
        if (!setSocketOptions()) {
            utils::Logger::error("Socket options set nahi hoe! Fer vi try karde haan...");
            // (Socket options didn't set! Still trying...)
        }

        // Bind to port
        if (!bindSocket()) {
            utils::Logger::error("Port te bind nahi ho sakda! Server fail.");
            // (Can't bind to port! Server failed.)
            close(serverSocket_);
            return;
        }

        // Listen for connections
        if (!listenForConnections()) {
            utils::Logger::error("Listen fail ho gaya! Server band kar ditte.");
            // (Listen failed! Shutting down server.)
            close(serverSocket_);
            return;
        }

        running_ = true;
        
        utils::Logger::info("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        utils::Logger::info("✅ Server successfully chaalu ho gaya! ✅");
        utils::Logger::info("🌐 Port 4221 te dhaba khul gaya, aao ji! 🌐");
        utils::Logger::info("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        // (Server successfully started!)
        // (Restaurant opened on port 4221, come on in!)

        // Main accept loop - clients nu accept karde raho
        // (Main accept loop - keep accepting clients)
        acceptLoop();
    }

    /**
     * Server band karo
     * (Stop the server)
     */
    void stop() {
        if (running_) {
            utils::Logger::info("Server band kar rahe haan...");
            // (Stopping server...)
            
            running_ = false;
            
            if (serverSocket_ >= 0) {
                close(serverSocket_);
                serverSocket_ = -1;
            }

            utils::Logger::info("Server completely band ho gaya. Dhaba band!");
            // (Server completely stopped. Restaurant closed!)
        }
    }

private:
    /**
     * Socket create karo
     * (Create socket)
     */
    bool createSocket() {
        serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
        
        if (serverSocket_ < 0) {
            utils::Logger::error("Socket creation fail! System vich problem hai.");
            // (Socket creation failed! System problem.)
            return false;
        }

        utils::Logger::debug("Socket successfully create ho gaya: FD " + std::to_string(serverSocket_));
        // (Socket successfully created)
        return true;
    }

    /**
     * Socket options set karo
     * (Set socket options)
     */
    bool setSocketOptions() {
        // SO_REUSEADDR - address reuse kar sakde haan (testing lai important!)
        // (SO_REUSEADDR - can reuse address, important for testing!)
        int reuse = 1;
        if (setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
            utils::Logger::warn("SO_REUSEADDR set nahi hoya! 'Address already in use' error aa sakda hai.");
            // (SO_REUSEADDR didn't set! 'Address already in use' error may occur.)
            return false;
        }

        utils::Logger::debug("Socket options set ho gaye (SO_REUSEADDR)");
        return true;
    }

    /**
     * Socket nu port te bind karo
     * (Bind socket to port)
     */
    bool bindSocket() {
        struct sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;  // Saare network interfaces te listen
                                                  // (Listen on all network interfaces)
        serverAddr.sin_port = htons(port_);       // Port number (network byte order)

        if (bind(serverSocket_, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) != 0) {
            utils::Logger::error("Port " + std::to_string(port_) + 
                               " te bind nahi ho sakda! Port already use vich ho sakda hai.");
            // (Can't bind to port! Port might already be in use.)
            return false;
        }

        utils::Logger::info("Socket successfully bind ho gaya port " + std::to_string(port_) + " te!");
        // (Socket successfully bound to port!)
        return true;
    }

    /**
     * Connections lai listen karo
     * (Listen for connections)
     */
    bool listenForConnections() {
        if (listen(serverSocket_, BACKLOG) != 0) {
            utils::Logger::error("Listen fail ho gaya! Socket setup vich problem.");
            // (Listen failed! Problem in socket setup.)
            return false;
        }

        utils::Logger::info("Server listen kar raha hai, backlog: " + std::to_string(BACKLOG));
        // (Server listening, backlog: X)
        return true;
    }

    /**
     * Main accept loop - clients accept karo te handle karo
     * (Main accept loop - accept and handle clients)
     */
    void acceptLoop() {
        utils::Logger::info("Accept loop shuru! Clients da wait kar rahe haan...");
        // (Accept loop started! Waiting for clients...)

        while (running_) {
            struct sockaddr_in clientAddr;
            socklen_t clientAddrLen = sizeof(clientAddr);

            // Client connection accept karo
            // (Accept client connection)
            int clientSocket = accept(serverSocket_, (struct sockaddr*)&clientAddr, &clientAddrLen);

            if (clientSocket < 0) {
                if (running_) {
                    utils::Logger::error("Accept fail! Client connection accept nahi ho sakda.");
                    // (Accept failed! Can't accept client connection.)
                }
                continue;
            }

            // Client info log karo
            // (Log client info)
            char clientIp[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &clientAddr.sin_addr, clientIp, INET_ADDRSTRLEN);
            
            utils::Logger::info("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
            utils::Logger::info("🎉 Naya client aa gaya ji! 🎉");
            utils::Logger::info("📍 IP: " + std::string(clientIp) + 
                              ", Socket: " + std::to_string(clientSocket));
            utils::Logger::info("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
            // (New client arrived!)

            // Thread pool vich task add karo
            // (Add task to thread pool)
            threadPool_->enqueue([this, clientSocket]() {
                handleClient(clientSocket);
            });
        }
    }

    /**
     * Individual client handle karo
     * (Handle individual client)
     * 
     * Called by worker thread from thread pool
     */
    void handleClient(int clientSocket) {
        try {
            // ConnectionHandler banao te connection handle karo
            // (Create ConnectionHandler and handle connection)
            handlers::ConnectionHandler handler(clientSocket, routeHandler_);
            handler.handleConnection();
            
        } catch (const std::exception& e) {
            utils::Logger::error("Client handling vich exception aa gayi: " + std::string(e.what()));
            // (Exception occurred while handling client)
            close(clientSocket);
        } catch (...) {
            utils::Logger::error("Client handling vich unknown exception!");
            // (Unknown exception while handling client)
            close(clientSocket);
        }
    }
};

} // namespace server
