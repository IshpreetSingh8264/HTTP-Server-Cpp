#include "handlers/ConnectionHandler.hpp"

// ConnectionHandler.cpp - Socket read/write, keep-alive loop, body framing.

#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <cstddef>
#include <string>

#include "http/HttpConstants.hpp"
#include "utils/Logger.hpp"
#include "utils/StringUtils.hpp"

namespace handlers {

ConnectionHandler::ConnectionHandler(int clientSocket,
                                     std::shared_ptr<RouteHandler> routeHandler)
    : clientSocket_(clientSocket), routeHandler_(std::move(routeHandler)) {
    setSocketTimeout();
    utils::Logger::debug("ConnectionHandler tayar, socket: " + std::to_string(clientSocket_));
}

ConnectionHandler::~ConnectionHandler() {
    closeConnection();
}

void ConnectionHandler::handleConnection() {
    utils::Logger::info("Client connection shuru! Socket: " + std::to_string(clientSocket_));

    // Persistent connection loop - jab tak client "Connection: close" nahi bhejda
    bool keepAlive = true;
    int requestCount = 0;

    while (keepAlive) {
        requestCount++;

        std::string rawRequest = readRequest();

        if (rawRequest.empty()) {
            utils::Logger::debug("Request khali aa gayi, connection band. Total: " +
                                 std::to_string(requestCount - 1));
            break;
        }

        http::HttpRequest request;
        if (!request.parse(rawRequest)) {
            utils::Logger::error("Request parse nahi ho sakdi! Bad request bhej rahe haan.");
            sendResponse(http::HttpResponse::badRequest("Invalid HTTP request"));
            break;
        }

        auto response = routeHandler_->handleRequest(request);

        const std::string connectionHeader =
            request.getHeader(http::HttpConstants::HEADER_CONNECTION);

        if (utils::StringUtils::equalsIgnoreCase(connectionHeader, http::HttpConstants::CONNECTION_CLOSE)) {
            keepAlive = false;
            response.setConnection(http::HttpConstants::CONNECTION_CLOSE);
        } else if (utils::StringUtils::equalsIgnoreCase(connectionHeader, http::HttpConstants::CONNECTION_KEEP_ALIVE)) {
            keepAlive = true;
            response.setConnection(http::HttpConstants::CONNECTION_KEEP_ALIVE);
        } else {
            // HTTP/1.1 vich default keep-alive hai
            keepAlive = true;
            response.setConnection(http::HttpConstants::CONNECTION_KEEP_ALIVE);
        }

        if (!sendResponse(response)) {
            utils::Logger::error("Response bhejn vich fail! Connection band kar ditte.");
            break;
        }

        utils::Logger::info("Request #" + std::to_string(requestCount) + " handle ho gayi!");
    }

    utils::Logger::info("Connection band ho gaya. Total requests: " + std::to_string(requestCount - 1));
}

void ConnectionHandler::setSocketTimeout() {
    struct timeval timeout;
    timeout.tv_sec = TIMEOUT_SECONDS;
    timeout.tv_usec = 0;

    if (setsockopt(clientSocket_, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        utils::Logger::warn("Socket timeout set nahi ho sakda. Chalta hai, continue karde haan.");
    } else {
        utils::Logger::debug("Socket read timeout set: " + std::to_string(TIMEOUT_SECONDS) + "s");
    }
}

std::string ConnectionHandler::readRequest() {
    std::string request;
    char buffer[BUFFER_SIZE];

    // Request headers read karde raho jab tak "\r\n\r\n" nahi milega
    while (true) {
        const ssize_t bytesRead = recv(clientSocket_, buffer, BUFFER_SIZE - 1, 0);

        if (bytesRead < 0) {
            utils::Logger::warn("Socket read error ya timeout. Connection band ho raha hai.");
            return "";
        }

        if (bytesRead == 0) {
            utils::Logger::debug("Client ne connection band kar ditti (0 bytes read).");
            return "";
        }

        request.append(buffer, static_cast<size_t>(bytesRead));

        const size_t headerEnd = request.find(http::HttpConstants::HEADER_SEPARATOR);

        if (headerEnd != std::string::npos) {
            http::HttpRequest tempReq;
            if (tempReq.parse(request)) {
                const std::string contentLengthStr =
                    tempReq.getHeader(http::HttpConstants::HEADER_CONTENT_LENGTH);

                if (!contentLengthStr.empty()) {
                    const size_t contentLength = std::stoull(contentLengthStr);
                    const size_t bodyStart = headerEnd + http::HttpConstants::HEADER_SEPARATOR_LENGTH;
                    size_t currentBodySize = request.size() - bodyStart;

                    while (currentBodySize < contentLength) {
                        const ssize_t bodyBytes = recv(clientSocket_, buffer, BUFFER_SIZE - 1, 0);

                        if (bodyBytes <= 0) {
                            utils::Logger::error("Body read karde time connection fail!");
                            return "";
                        }

                        request.append(buffer, static_cast<size_t>(bodyBytes));
                        currentBodySize += static_cast<size_t>(bodyBytes);
                    }
                }
            }

            break;
        }

        if (request.size() > MAX_REQUEST_SIZE) {
            utils::Logger::error("Request bohot vaddi hai! 1MB se jyada. Reject kar ditte.");
            return "";
        }
    }

    utils::Logger::debug("Request read ho gayi, size: " + std::to_string(request.size()) + " bytes");
    return request;
}

bool ConnectionHandler::sendResponse(const http::HttpResponse& response) {
    const std::string responseStr = response.toString();

    utils::Logger::debug("Response bhej rahe haan, size: " + std::to_string(responseStr.size()));

    size_t totalSent = 0;
    while (totalSent < responseStr.size()) {
        const ssize_t sent = send(clientSocket_, responseStr.data() + totalSent,
                                  responseStr.size() - totalSent, 0);

        if (sent < 0) {
            utils::Logger::error("Response bhejn vich error! Socket problem.");
            return false;
        }

        totalSent += static_cast<size_t>(sent);
    }

    utils::Logger::debug("Response successfully bhej ditti!");
    return true;
}

void ConnectionHandler::closeConnection() {
    if (clientSocket_ >= 0) {
        close(clientSocket_);
        utils::Logger::debug("Socket band ho gaya: " + std::to_string(clientSocket_));
        clientSocket_ = -1;
    }
}

} // namespace handlers
