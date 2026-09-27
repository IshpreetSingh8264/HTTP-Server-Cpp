#include "handlers/RouteHandler.hpp"

// RouteHandler.cpp - Route matching for every endpoint.
// TODO(item 2): this if-chain moves into a data-driven route table under routes/.

#include <string>

#include "http/HttpConstants.hpp"
#include "compression/GzipCompressor.hpp"
#include "utils/Logger.hpp"
#include "utils/StringUtils.hpp"

namespace handlers {

RouteHandler::RouteHandler(std::shared_ptr<FileHandler> fileHandler)
    : fileHandler_(std::move(fileHandler)) {
    utils::Logger::info("RouteHandler tayar hai!");
}

http::HttpResponse RouteHandler::handleRequest(const http::HttpRequest& request) {
    const std::string& method = request.getMethod();
    const std::string& path = request.getPath();

    utils::Logger::info("Request aa gayi: " + method + " " + path);

    if (method == http::HttpConstants::METHOD_GET) {
        return handleGetRequest(request);
    } else if (method == http::HttpConstants::METHOD_POST) {
        return handlePostRequest(request);
    } else {
        // Method supported nahi hai
        utils::Logger::warn("Method " + method + " supported nahi hai!");
        return http::HttpResponse::badRequest("Method " + method + " not supported");
    }
}

http::HttpResponse RouteHandler::handleGetRequest(const http::HttpRequest& request) {
    const std::string& path = request.getPath();

    if (path == "/") {
        return handleRoot(request);
    }

    if (utils::StringUtils::startsWith(path, "/echo/")) {
        return handleEcho(request);
    }

    if (path == "/user-agent") {
        return handleUserAgent(request);
    }

    if (utils::StringUtils::startsWith(path, "/files/")) {
        return handleFileGet(request);
    }

    utils::Logger::warn("Path nahi mileya: " + path);
    return http::HttpResponse::notFound("Path not found: " + path);
}

http::HttpResponse RouteHandler::handlePostRequest(const http::HttpRequest& request) {
    const std::string& path = request.getPath();

    if (utils::StringUtils::startsWith(path, "/files/")) {
        return handleFilePost(request);
    }

    return http::HttpResponse::notFound("POST endpoint not found: " + path);
}

http::HttpResponse RouteHandler::handleRoot(const http::HttpRequest& /*request*/) {
    utils::Logger::debug("Root endpoint hit hoyi!");
    return http::HttpResponse::ok("");
}

http::HttpResponse RouteHandler::handleEcho(const http::HttpRequest& request) {
    const std::string& path = request.getPath();

    // "/echo/" ke baad ka string extract karo (6 = length of "/echo/")
    std::string echoStr = path.substr(6);

    utils::Logger::debug("Echo request: '" + echoStr + "'");

    http::HttpResponse response = http::HttpResponse::ok(echoStr, http::HttpConstants::MIME_TEXT_PLAIN);
    applyCompression(response, echoStr, request);
    return response;
}

http::HttpResponse RouteHandler::handleUserAgent(const http::HttpRequest& request) {
    std::string userAgent = request.getHeader(http::HttpConstants::HEADER_USER_AGENT);

    if (userAgent.empty()) {
        utils::Logger::warn("User-Agent header nahi mili!");
        userAgent = "Unknown";
    }

    http::HttpResponse response = http::HttpResponse::ok(userAgent, http::HttpConstants::MIME_TEXT_PLAIN);
    applyCompression(response, userAgent, request);
    return response;
}

http::HttpResponse RouteHandler::handleFileGet(const http::HttpRequest& request) {
    const std::string& path = request.getPath();

    // Filename extract karo (7 = length of "/files/")
    std::string filename = path.substr(7);

    utils::Logger::info("File request aa gayi: " + filename);

    if (!fileHandler_->fileExists(filename)) {
        utils::Logger::warn("File nahi mili: " + filename);
        return http::HttpResponse::notFound("File not found: " + filename);
    }

    auto fileData = fileHandler_->readFile(filename);

    if (fileData.empty()) {
        utils::Logger::error("File read ho nahi sakdi: " + filename);
        return http::HttpResponse::internalError("Cannot read file");
    }

    http::HttpResponse response(http::HttpConstants::STATUS_OK);
    response.setContentType(http::HttpConstants::getMimeType(filename));
    response.setBody(fileData);

    // Text files lai compression apply kar sakde haan
    applyCompression(response, std::string(fileData.begin(), fileData.end()), request);

    utils::Logger::info("File bhej ditti: " + filename);
    return response;
}

http::HttpResponse RouteHandler::handleFilePost(const http::HttpRequest& request) {
    const std::string& path = request.getPath();

    // Filename extract karo (7 = length of "/files/")
    std::string filename = path.substr(7);

    utils::Logger::info("File upload request: " + filename);

    const std::string& body = request.getBody();

    if (body.empty()) {
        utils::Logger::warn("Empty body file save karn lai bheji!");
        return http::HttpResponse::badRequest("Empty file content");
    }

    if (fileHandler_->writeFile(filename, body)) {
        utils::Logger::info("File save ho gayi: " + filename +
                            " (" + std::to_string(body.size()) + " bytes)");
        return http::HttpResponse::created("", http::HttpConstants::MIME_TEXT_PLAIN);
    }

    utils::Logger::error("File save nahi ho sakdi: " + filename);
    return http::HttpResponse::internalError("Cannot save file");
}

void RouteHandler::applyCompression(http::HttpResponse& response, const std::string& data,
                                    const http::HttpRequest& request) {
    const std::string acceptEncoding =
        request.getHeader(http::HttpConstants::HEADER_ACCEPT_ENCODING);

    if (acceptEncoding.empty()) {
        return;
    }

    if (!compression::GzipCompressor::supportsGzip(acceptEncoding)) {
        return;
    }

    // Response da Content-Type check karo (request da nahi!)
    std::string contentType = response.getHeader(http::HttpConstants::HEADER_CONTENT_TYPE);
    if (contentType.empty()) {
        contentType = http::HttpConstants::MIME_TEXT_PLAIN;
    }

    if (!compression::GzipCompressor::shouldCompress(data, contentType)) {
        return;
    }

    auto compressed = compression::GzipCompressor::compress(data);

    if (!compressed.empty()) {
        response.setCompressedBody(compressed, http::HttpConstants::ENCODING_GZIP);
        utils::Logger::info("Response gzip compressed bhej ditti!");
    }
}

} // namespace handlers
