#include "handlers/ConnectionHandler.hpp"

// ConnectionHandler.cpp - Socket read/write, request framing, keep-alive loop.

#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <cerrno>
#include <exception>
#include <string>

#include "http/HttpConstants.hpp"
#include "utils/Logger.hpp"
#include "utils/StringUtils.hpp"

namespace handlers {

namespace {

/**
 * Parse a Content-Length header value.
 *
 * std::stoull alone is not good enough here, and each of its failure modes
 * was a live bug:
 *   - "abc"        throws std::invalid_argument, which used to escape
 *                  readRequest() and kill the worker thread with no reply
 *   - "-5"         does not throw at all; it wraps to 18446744073709551611,
 *                  so the server waited ~19 exabytes for a body that was
 *                  never coming
 *   - "9e99..."    throws std::out_of_range, same silent death as "abc"
 *
 * So: digits only, no sign, no exponent, and bounded by MAX_REQUEST_SIZE.
 */
bool parseContentLength(const std::string& rawValue, size_t maxAllowed, size_t& out) {
    const std::string value = utils::StringUtils::trim(rawValue);

    if (value.empty()) {
        return false;
    }
    for (const char c : value) {
        if (c < '0' || c > '9') {
            return false;
        }
    }

    errno = 0;
    try {
        const unsigned long long parsed = std::stoull(value);
        if (errno == ERANGE || parsed > static_cast<unsigned long long>(maxAllowed)) {
            return false;
        }
        out = static_cast<size_t>(parsed);
        return true;
    } catch (const std::exception&) {
        // Unreachable given the digit check, but stoull is allowed to throw.
        return false;
    }
}

} // namespace

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
    int served = 0;

    while (keepAlive) {
        std::string rawRequest;
        const ReadResult read = readRequest(rawRequest);

        if (read == ReadResult::Closed) {
            utils::Logger::debug("Connection khatam. Total requests served: " +
                                 std::to_string(served));
            return;
        }

        if (read == ReadResult::Invalid) {
            utils::Logger::warn("Malformed request framing! 400 bhej ke band kar rahe haan.");
            sendResponse(http::HttpResponse::badRequest("Invalid HTTP request"));
            return;
        }

        http::HttpRequest request;
        if (!request.parse(rawRequest)) {
            utils::Logger::error("Request parse nahi ho sakdi! Bad request bhej rahe haan.");
            sendResponse(http::HttpResponse::badRequest("Invalid HTTP request"));
            return;
        }

        http::HttpResponse response = routeHandler_->handleRequest(request);

        const std::string connectionHeader =
            request.getHeader(http::HttpConstants::HEADER_CONNECTION);

        if (utils::StringUtils::equalsIgnoreCase(connectionHeader,
                                                 http::HttpConstants::CONNECTION_CLOSE)) {
            keepAlive = false;
            response.setConnection(http::HttpConstants::CONNECTION_CLOSE);
        } else if (utils::StringUtils::equalsIgnoreCase(
                       connectionHeader, http::HttpConstants::CONNECTION_KEEP_ALIVE)) {
            keepAlive = true;
            response.setConnection(http::HttpConstants::CONNECTION_KEEP_ALIVE);
        } else {
            // HTTP/1.1 vich default keep-alive hai
            keepAlive = true;
            response.setConnection(http::HttpConstants::CONNECTION_KEEP_ALIVE);
        }

        if (!sendResponse(response)) {
            utils::Logger::error("Response bhejn vich fail! Connection band kar ditte.");
            return;
        }

        served++;
    }

    utils::Logger::info("Connection band ho gaya. Total requests served: " +
                        std::to_string(served));
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

ConnectionHandler::ReadResult ConnectionHandler::readRequest(std::string& outRequest) {
    outRequest.clear();

    while (true) {
        // 1. Can we carve a whole request out of what we already hold?
        const size_t headerEnd = readBuffer_.find(http::HttpConstants::HEADER_SEPARATOR);

        if (headerEnd != std::string::npos) {
            const size_t bodyStart = headerEnd + http::HttpConstants::HEADER_SEPARATOR_LENGTH;

            http::HttpRequest probe;
            size_t contentLength = 0;
            bool hasBody = false;

            if (probe.parse(readBuffer_)) {
                const std::string declared =
                    probe.getHeader(http::HttpConstants::HEADER_CONTENT_LENGTH);

                if (!declared.empty()) {
                    if (!parseContentLength(declared, MAX_REQUEST_SIZE, contentLength)) {
                        utils::Logger::warn("Content-Length invalid ya bohot Bada: '" +
                                            declared + "'");
                        return ReadResult::Invalid;
                    }
                    hasBody = true;
                }
            }

            const size_t needed = bodyStart + (hasBody ? contentLength : 0);

            if (readBuffer_.size() >= needed) {
                // Take exactly one request. Anything after it is the next one.
                outRequest = readBuffer_.substr(0, needed);
                readBuffer_.erase(0, needed);

                utils::Logger::debug("Request read ho gayi, size: " +
                                     std::to_string(outRequest.size()) + " bytes, buffered: " +
                                     std::to_string(readBuffer_.size()));
                return ReadResult::Ok;
            }
        }

        // 2. Need more bytes from the wire.
        if (readBuffer_.size() > MAX_REQUEST_SIZE) {
            utils::Logger::error("Request bohot vaddi hai! 1MB se jyada. Reject kar ditte.");
            return ReadResult::Closed;
        }

        char buffer[BUFFER_SIZE];
        const ssize_t bytesRead = recv(clientSocket_, buffer, sizeof(buffer), 0);

        if (bytesRead < 0) {
            if (errno == EINTR) {
                continue;  // Interrupted by a signal, not a real error.
            }
            // EAGAIN/EWOULDBLOCK here means the read timeout expired.
            utils::Logger::debug("Socket read error ya timeout. Connection band ho raha hai.");
            return ReadResult::Closed;
        }

        if (bytesRead == 0) {
            utils::Logger::debug("Client ne connection band kar ditti (0 bytes read).");
            return ReadResult::Closed;
        }

        readBuffer_.append(buffer, static_cast<size_t>(bytesRead));
    }
}

bool ConnectionHandler::sendResponse(const http::HttpResponse& response) {
    const std::string responseStr = response.toString();

    utils::Logger::debug("Response bhej rahe haan, size: " + std::to_string(responseStr.size()));

    size_t totalSent = 0;
    while (totalSent < responseStr.size()) {
        // MSG_NOSIGNAL: writing to a peer that already went away must return
        // EPIPE. Without it the default SIGPIPE disposition kills the whole
        // server, taking every other in-flight connection with it.
        const ssize_t sent = send(clientSocket_, responseStr.data() + totalSent,
                                  responseStr.size() - totalSent, MSG_NOSIGNAL);

        if (sent < 0) {
            if (errno == EINTR) {
                continue;
            }
            utils::Logger::error("Response bhejn vich error! Socket problem.");
            return false;
        }

        if (sent == 0) {
            // No progress and no error: treat as a dead peer rather than spin.
            utils::Logger::error("send() returned 0; peer is gone.");
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
