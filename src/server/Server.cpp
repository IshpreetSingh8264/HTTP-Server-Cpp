#include "server/Server.hpp"

// Server.cpp - Listening socket, accept loop, task dispatch.

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <exception>

#include "handlers/ConnectionHandler.hpp"
#include "utils/Logger.hpp"

namespace server {

Server::Server(int port, const std::string& fileDir)
    : serverSocket_(-1),
      port_(port),
      fileDirectory_(fileDir),
      running_(false) {
    utils::Logger::info("Server setup shuru kar rahe haan...");

    // Component graph, wired bottom-up.
    fileHandler_ = std::make_shared<handlers::FileHandler>(fileDirectory_);
    routeHandler_ = std::make_shared<handlers::RouteHandler>(fileHandler_);
    threadPool_ = std::make_unique<ThreadPool>();

    utils::Logger::info("Server components tayar hain! Port: " + std::to_string(port_));
}

Server::~Server() {
    stop();
}

void Server::start() {
    utils::Logger::info("Server chaalu kar rahe haan port " + std::to_string(port_) + " te...");

    if (!createSocket()) {
        utils::Logger::error("Oye! Socket ni baneya, server start nahi ho sakda!");
        return;
    }

    if (!setSocketOptions()) {
        utils::Logger::error("Socket options set nahi hoe! Fer vi try karde haan...");
    }

    if (!bindSocket()) {
        utils::Logger::error("Port te bind nahi ho sakda! Server fail.");
        close(serverSocket_);
        serverSocket_ = -1;
        return;
    }

    if (!listenForConnections()) {
        utils::Logger::error("Listen fail ho gaya! Server band kar ditte.");
        close(serverSocket_);
        serverSocket_ = -1;
        return;
    }

    running_ = true;

    utils::Logger::info("Server successfully chaalu ho gaya! Port " + std::to_string(port_));

    acceptLoop();  // blocking
}

void Server::stop() {
    if (running_.exchange(false)) {
        utils::Logger::info("Server band kar rahe haan...");

        if (serverSocket_ >= 0) {
            // Shutdown() first so a blocked accept() returns instead of hanging.
            shutdown(serverSocket_, SHUT_RDWR);
            close(serverSocket_);
            serverSocket_ = -1;
        }

        utils::Logger::info("Server completely band ho gaya. Dhaba band!");
    }
}

bool Server::createSocket() {
    serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);

    if (serverSocket_ < 0) {
        utils::Logger::error("Socket creation fail! System vich problem hai.");
        return false;
    }

    utils::Logger::debug("Socket create ho gaya: FD " + std::to_string(serverSocket_));
    return true;
}

bool Server::setSocketOptions() {
    int reuse = 1;
    if (setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        utils::Logger::warn("SO_REUSEADDR set nahi hoya! 'Address already in use' aa sakda hai.");
        return false;
    }

    utils::Logger::debug("Socket options set ho gaye (SO_REUSEADDR)");
    return true;
}

bool Server::bindSocket() {
    struct sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;  // All network interfaces
    serverAddr.sin_port = htons(static_cast<uint16_t>(port_));

    if (bind(serverSocket_, reinterpret_cast<struct sockaddr*>(&serverAddr), sizeof(serverAddr)) != 0) {
        utils::Logger::error("Port " + std::to_string(port_) + " te bind nahi ho sakda! " +
                             std::strerror(errno));
        return false;
    }

    utils::Logger::info("Socket successfully bind ho gaya port " + std::to_string(port_) + " te!");
    return true;
}

bool Server::listenForConnections() {
    if (listen(serverSocket_, BACKLOG) != 0) {
        utils::Logger::error("Listen fail ho gaya! Socket setup vich problem.");
        return false;
    }

    utils::Logger::info("Server listen kar raha hai, backlog: " + std::to_string(BACKLOG));
    return true;
}

void Server::acceptLoop() {
    utils::Logger::info("Accept loop shuru! Clients da wait kar rahe haan...");

    while (running_) {
        struct sockaddr_in clientAddr{};
        socklen_t clientAddrLen = sizeof(clientAddr);

        const int clientSocket =
            accept(serverSocket_, reinterpret_cast<struct sockaddr*>(&clientAddr), &clientAddrLen);

        if (clientSocket < 0) {
            if (errno == EINTR) {
                continue;
            }
            if (running_) {
                utils::Logger::error("Accept fail! Client connection accept nahi ho sakda.");
            }
            // The listening socket was closed underneath us; stop spinning.
            if (!running_ || errno == EBADF || errno == EINVAL) {
                break;
            }
            continue;
        }

        char clientIp[INET_ADDRSTRLEN] = "unknown";
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIp, INET_ADDRSTRLEN);
        utils::Logger::info("Naya client aa gaya! IP: " + std::string(clientIp) +
                            ", Socket: " + std::to_string(clientSocket));

        threadPool_->enqueue([this, clientSocket]() {
            handleClient(clientSocket);
        });
    }

    utils::Logger::info("Accept loop khatam.");
}

void Server::handleClient(int clientSocket) {
    try {
        handlers::ConnectionHandler handler(clientSocket, routeHandler_);
        handler.handleConnection();
    } catch (const std::exception& e) {
        utils::Logger::error("Client handling vich exception: " + std::string(e.what()));
        close(clientSocket);
    } catch (...) {
        utils::Logger::error("Client handling vich unknown exception!");
        close(clientSocket);
    }
}

} // namespace server
