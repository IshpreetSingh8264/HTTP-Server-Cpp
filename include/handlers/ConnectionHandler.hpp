#pragma once

// ConnectionHandler.hpp - Client connection management
// Socket connection nu handle karo - read, write, timeout, persistence!
// (Handle socket connection - read, write, timeout, persistence!)

#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <string>
#include <memory>
#include "http/HttpRequest.hpp"
#include "http/HttpResponse.hpp"
#include "http/HttpConstants.hpp"
#include "handlers/RouteHandler.hpp"
#include "utils/Logger.hpp"
#include "utils/StringUtils.hpp"

namespace handlers {

/**
 * ConnectionHandler - Handle individual client connections
 * 
 * Purpose: Socket lifecycle manage karo - accept te close tak
 *          (Manage socket lifecycle - from accept to close)
 * 
 * Features:
 * - Read HTTP requests from socket
 * - Send HTTP responses to socket
 * - Persistent connections (Connection: keep-alive)
 * - Read timeout (30 seconds standard)
 * - Graceful connection closure
 */
class ConnectionHandler {
private:
    int clientSocket_;                          // Client socket file descriptor
    std::shared_ptr<RouteHandler> routeHandler_; // Request routing lai
                                                // (For request routing)
    
    static constexpr int BUFFER_SIZE = 8192;    // 8KB read buffer
    static constexpr int TIMEOUT_SECONDS = 30;   // 30 second timeout (HTTP/1.1 standard)
                                                 // (30 second timeout)

public:
    /**
     * Constructor - Client socket te route handler inject karo
     * (Constructor - Inject client socket and route handler)
     */
    ConnectionHandler(int clientSocket, std::shared_ptr<RouteHandler> routeHandler)
        : clientSocket_(clientSocket), routeHandler_(routeHandler) {
        
        // Socket timeout set karo
        // (Set socket timeout)
        setSocketTimeout();
        
        utils::Logger::debug("ConnectionHandler tayar, socket: " + std::to_string(clientSocket_));
        // (ConnectionHandler ready)
    }

    /**
     * Destructor - Socket band kar do
     * (Destructor - Close socket)
     */
    ~ConnectionHandler() {
        closeConnection();
    }

    /**
     * Client connection handle karo
     * (Handle client connection)
     * 
     * Persistent connection support:
     * - Loop while "Connection: keep-alive"
     * - Read multiple requests on same socket
     * - Timeout after 30 seconds of inactivity
     */
    void handleConnection() {
        utils::Logger::info("Client connection shuru! Socket: " + std::to_string(clientSocket_));
        // (Client connection started!)

        // Persistent connection loop
        // Jab tak client "Connection: close" nahi bhejda, requests handle karde raho
        // (Keep handling requests until client sends "Connection: close")
        bool keepAlive = true;
        int requestCount = 0;

        while (keepAlive) {
            requestCount++;
            
            // Request read karo
            // (Read request)
            std::string rawRequest = readRequest();
            
            if (rawRequest.empty()) {
                // Timeout ho gaya ya connection band ho gaya
                // (Timeout occurred or connection closed)
                utils::Logger::debug("Request khali aa gayi, connection band kar ditte. Total requests: " + 
                                   std::to_string(requestCount - 1));
                // (Empty request received, closing connection)
                break;
            }

            // Request parse karo
            // (Parse request)
            http::HttpRequest request;
            if (!request.parse(rawRequest)) {
                // Parsing fail - bad request bhejo te connection band karo
                // (Parsing failed - send bad request and close connection)
                utils::Logger::error("Request parse nahi ho sakdi! Bad request bhej rahe haan.");
                // (Request couldn't be parsed! Sending bad request.)
                
                auto response = http::HttpResponse::badRequest("Invalid HTTP request");
                sendResponse(response);
                break;
            }

            // Request handle karke response banao
            // (Handle request and create response)
            auto response = routeHandler_->handleRequest(request);
            
            // Connection header check karo - keep-alive hai ya close?
            // (Check Connection header - keep-alive or close?)
            std::string connectionHeader = request.getHeader(http::HttpConstants::HEADER_CONNECTION);
            
            if (utils::StringUtils::equalsIgnoreCase(connectionHeader, http::HttpConstants::CONNECTION_CLOSE)) {
                // Client ne close request kitti
                // (Client requested close)
                keepAlive = false;
                response.setConnection(http::HttpConstants::CONNECTION_CLOSE);
                utils::Logger::debug("Client ne Connection: close bhejeya, band kar rahe haan.");
                // (Client sent Connection: close, closing.)
            } else if (utils::StringUtils::equalsIgnoreCase(connectionHeader, http::HttpConstants::CONNECTION_KEEP_ALIVE)) {
                // Keep-alive request
                keepAlive = true;
                response.setConnection(http::HttpConstants::CONNECTION_KEEP_ALIVE);
                utils::Logger::debug("Connection keep-alive, agla request wait kar rahe haan.");
                // (Connection keep-alive, waiting for next request.)
            } else {
                // HTTP/1.1 vich default keep-alive hai
                // (In HTTP/1.1 default is keep-alive)
                keepAlive = true;
                response.setConnection(http::HttpConstants::CONNECTION_KEEP_ALIVE);
            }

            // Response bhejo
            // (Send response)
            if (!sendResponse(response)) {
                utils::Logger::error("Response bhejn vich fail! Connection band kar ditte.");
                // (Failed to send response! Closing connection.)
                break;
            }

            utils::Logger::info("Request #" + std::to_string(requestCount) + " successfully handle ho gayi!");
            // (Request successfully handled!)
        }

        utils::Logger::info("Connection band ho gaya. Total requests handled: " + std::to_string(requestCount - 1));
        // (Connection closed. Total requests handled)
    }

private:
    /**
     * Socket timeout set karo
     * (Set socket timeout)
     * 
     * 30 second read timeout - agar client response nahi denda toh disconnect
     * (30 second read timeout - disconnect if client doesn't respond)
     */
    void setSocketTimeout() {
        struct timeval timeout;
        timeout.tv_sec = TIMEOUT_SECONDS;
        timeout.tv_usec = 0;

        // Read timeout set karo
        // (Set read timeout)
        if (setsockopt(clientSocket_, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
            utils::Logger::warn("Oye! Socket timeout set nahi ho sakda. Chalta hai, continue karde haan.");
            // (Hey! Couldn't set socket timeout. It's okay, continuing.)
        } else {
            utils::Logger::debug("Socket read timeout set: " + std::to_string(TIMEOUT_SECONDS) + " seconds");
        }
    }

    /**
     * HTTP request read karo socket se
     * (Read HTTP request from socket)
     * 
     * @return: Complete HTTP request as string, empty if error/timeout
     */
    std::string readRequest() {
        std::string request;
        char buffer[BUFFER_SIZE];
        
        // Request headers read karde raho jab tak "\r\n\r\n" nahi milega
        // (Keep reading request headers until "\r\n\r\n" is found)
        while (true) {
            ssize_t bytesRead = recv(clientSocket_, buffer, BUFFER_SIZE - 1, 0);
            
            if (bytesRead < 0) {
                // Error ya timeout
                utils::Logger::warn("Socket read error ya timeout. Connection band ho raha hai.");
                // (Socket read error or timeout. Closing connection.)
                return "";
            }
            
            if (bytesRead == 0) {
                // Client ne connection band kar ditti
                // (Client closed connection)
                utils::Logger::debug("Client ne connection band kar ditti (0 bytes read).");
                return "";
            }

            buffer[bytesRead] = '\0';
            request.append(buffer, bytesRead);

            // Header end marker check karo
            // (Check for header end marker)
            size_t headerEnd = request.find(http::HttpConstants::HEADER_SEPARATOR);
            
            if (headerEnd != std::string::npos) {
                // Headers mil gayi! Body vi read karni hai?
                // (Headers found! Need to read body too?)
                
                // Content-Length check karo
                // (Check Content-Length)
                http::HttpRequest tempReq;
                if (tempReq.parse(request)) {
                    std::string contentLengthStr = tempReq.getHeader(http::HttpConstants::HEADER_CONTENT_LENGTH);
                    
                    if (!contentLengthStr.empty()) {
                        // Body expected hai - Content-Length de hisaab se read karo
                        // (Body expected - read according to Content-Length)
                        size_t contentLength = std::stoull(contentLengthStr);
                        size_t bodyStart = headerEnd + 4;  // "\r\n\r\n" = 4 bytes
                        size_t currentBodySize = request.size() - bodyStart;
                        
                        // Remaining body read karo
                        // (Read remaining body)
                        while (currentBodySize < contentLength) {
                            bytesRead = recv(clientSocket_, buffer, BUFFER_SIZE - 1, 0);
                            
                            if (bytesRead <= 0) {
                                utils::Logger::error("Body read karde time connection fail!");
                                // (Connection failed while reading body!)
                                return "";
                            }
                            
                            request.append(buffer, bytesRead);
                            currentBodySize += bytesRead;
                        }
                    }
                }
                
                // Complete request mil gayi!
                // (Complete request received!)
                break;
            }

            // Agar request bohot vaddi ho gayi toh error
            // (If request too large, error)
            if (request.size() > 1024 * 1024) {  // 1MB limit
                utils::Logger::error("Request bohot vaddi hai! 1MB se jyada. Reject kar ditte.");
                // (Request too large! More than 1MB. Rejecting.)
                return "";
            }
        }

        utils::Logger::debug("Request read ho gayi, size: " + std::to_string(request.size()) + " bytes");
        // (Request read, size: X bytes)

        return request;
    }

    /**
     * HTTP response bhejo socket te
     * (Send HTTP response to socket)
     * 
     * @param response: HttpResponse object
     * @return: true if successful
     */
    bool sendResponse(const http::HttpResponse& response) {
        std::string responseStr = response.toString();
        
        utils::Logger::debug("Response bhej rahe haan, size: " + std::to_string(responseStr.size()) + " bytes");
        // (Sending response)

        // Complete response bhejo
        // (Send complete response)
        ssize_t totalSent = 0;
        ssize_t remaining = responseStr.size();
        
        while (remaining > 0) {
            ssize_t sent = send(clientSocket_, responseStr.data() + totalSent, remaining, 0);
            
            if (sent < 0) {
                utils::Logger::error("Response bhejn vich error! Socket problem.");
                // (Error sending response! Socket problem.)
                return false;
            }
            
            totalSent += sent;
            remaining -= sent;
        }

        utils::Logger::debug("Response successfully bhej ditti!");
        // (Response successfully sent!)
        
        return true;
    }

    /**
     * Connection band karo
     * (Close connection)
     */
    void closeConnection() {
        if (clientSocket_ >= 0) {
            close(clientSocket_);
            utils::Logger::debug("Socket band ho gaya: " + std::to_string(clientSocket_));
            // (Socket closed)
            clientSocket_ = -1;
        }
    }
};

} // namespace handlers
