#pragma once

// RouteHandler.hpp - URL routing te request handling
// Different URLs lai different responses!
// (Different responses for different URLs!)

#include <string>
#include <memory>
#include "http/HttpRequest.hpp"
#include "http/HttpResponse.hpp"
#include "http/HttpConstants.hpp"
#include "handlers/FileHandler.hpp"
#include "compression/GzipCompressor.hpp"
#include "utils/Logger.hpp"
#include "utils/StringUtils.hpp"

namespace handlers {

/**
 * RouteHandler - Handle different HTTP routes
 * 
 * Purpose: URL path de hisaab se appropriate response bhejo
 *          (Send appropriate response based on URL path)
 * 
 * Supported routes:
 * - GET /              -> 200 OK
 * - GET /echo/{str}    -> Echo back the string
 * - GET /user-agent    -> Return User-Agent header
 * - GET /files/{name}  -> Return file contents
 * - POST /files/{name} -> Save file
 */
class RouteHandler {
private:
    std::shared_ptr<FileHandler> fileHandler_;  // File operations lai
                                               // (For file operations)

public:
    /**
     * Constructor - FileHandler inject karo
     * (Constructor - Inject FileHandler)
     */
    explicit RouteHandler(std::shared_ptr<FileHandler> fileHandler) 
        : fileHandler_(fileHandler) {
        utils::Logger::info("RouteHandler tayar hai!");
        // (RouteHandler ready!)
    }

    /**
     * Request handle karo te response bana do
     * (Handle request and create response)
     * 
     * @param request: Parsed HTTP request
     * @return: HTTP response
     */
    http::HttpResponse handleRequest(const http::HttpRequest& request) {
        const std::string& method = request.getMethod();
        const std::string& path = request.getPath();

        utils::Logger::info("Request aa gayi: " + method + " " + path);
        // (Request received)

        // Method de hisaab se route karo
        // (Route based on method)
        if (method == http::HttpConstants::METHOD_GET) {
            return handleGetRequest(request);
        } else if (method == http::HttpConstants::METHOD_POST) {
            return handlePostRequest(request);
        } else {
            // Method supported nahi hai
            // (Method not supported)
            utils::Logger::warn("Method " + method + " supported nahi hai!");
            return http::HttpResponse::badRequest("Method " + method + " not supported");
        }
    }

private:
    /**
     * GET requests handle karo
     * (Handle GET requests)
     */
    http::HttpResponse handleGetRequest(const http::HttpRequest& request) {
        const std::string& path = request.getPath();

        // Route matching - kon sa path hai?
        // (Route matching - which path is it?)
        
        // 1. Root path: GET /
        if (path == "/") {
            return handleRoot(request);
        }
        
        // 2. Echo endpoint: GET /echo/{str}
        if (utils::StringUtils::startsWith(path, "/echo/")) {
            return handleEcho(request);
        }
        
        // 3. User-Agent endpoint: GET /user-agent
        if (path == "/user-agent") {
            return handleUserAgent(request);
        }
        
        // 4. File serving: GET /files/{filename}
        if (utils::StringUtils::startsWith(path, "/files/")) {
            return handleFileGet(request);
        }

        // Koi route match nahi hoya - 404!
        // (No route matched - 404!)
        utils::Logger::warn("Path nahi mileya: " + path);
        // (Path not found)
        return http::HttpResponse::notFound("Path not found: " + path);
    }

    /**
     * POST requests handle karo
     * (Handle POST requests)
     */
    http::HttpResponse handlePostRequest(const http::HttpRequest& request) {
        const std::string& path = request.getPath();

        // File upload: POST /files/{filename}
        if (utils::StringUtils::startsWith(path, "/files/")) {
            return handleFilePost(request);
        }

        // POST lai koi hor route nahi
        // (No other routes for POST)
        return http::HttpResponse::notFound("POST endpoint not found: " + path);
    }

    /**
     * Root endpoint handler - GET /
     * (Handle root endpoint)
     */
    http::HttpResponse handleRoot(const http::HttpRequest& request) {
        utils::Logger::debug("Root endpoint hit hoyi!");
        // (Root endpoint hit!)
        
        auto response = http::HttpResponse::ok("");
        return response;
    }

    /**
     * Echo endpoint handler - GET /echo/{str}
     * (Handle echo endpoint)
     * 
     * Returns the string from URL path
     * Example: GET /echo/hello -> Response body: "hello"
     */
    http::HttpResponse handleEcho(const http::HttpRequest& request) {
        const std::string& path = request.getPath();
        
        // "/echo/" ke baad ka string extract karo
        // (Extract string after "/echo/")
        std::string echoStr = path.substr(6);  // 6 = length of "/echo/"
        
        utils::Logger::debug("Echo request: '" + echoStr + "'");

        // Response banao
        // (Create response)
        http::HttpResponse response = http::HttpResponse::ok(echoStr, http::HttpConstants::MIME_TEXT_PLAIN);
        
        // Compression check karo
        // (Check for compression)
        applyCompression(response, echoStr, request);
        
        return response;
    }

    /**
     * User-Agent endpoint handler - GET /user-agent
     * (Handle user-agent endpoint)
     * 
     * Returns the User-Agent header value
     */
    http::HttpResponse handleUserAgent(const http::HttpRequest& request) {
        std::string userAgent = request.getHeader(http::HttpConstants::HEADER_USER_AGENT);
        
        if (userAgent.empty()) {
            utils::Logger::warn("User-Agent header nahi mili!");
            // (User-Agent header not found!)
            userAgent = "Unknown";
        }

        utils::Logger::debug("User-Agent: " + userAgent);
        
        http::HttpResponse response = http::HttpResponse::ok(userAgent, http::HttpConstants::MIME_TEXT_PLAIN);
        
        // Compression apply karo agar client support karda hai
        // (Apply compression if client supports)
        applyCompression(response, userAgent, request);
        
        return response;
    }

    /**
     * File GET handler - GET /files/{filename}
     * (Handle file retrieval)
     */
    http::HttpResponse handleFileGet(const http::HttpRequest& request) {
        const std::string& path = request.getPath();
        
        // Filename extract karo
        // (Extract filename)
        std::string filename = path.substr(7);  // 7 = length of "/files/"
        
        utils::Logger::info("File request aa gayi: " + filename);
        // (File request received)

        // File exist kardi hai?
        // (Does file exist?)
        if (!fileHandler_->fileExists(filename)) {
            utils::Logger::warn("File nahi mili: " + filename);
            // (File not found)
            return http::HttpResponse::notFound("File not found: " + filename);
        }

        // File read karo
        // (Read file)
        auto fileData = fileHandler_->readFile(filename);
        
        if (fileData.empty()) {
            utils::Logger::error("File read ho nahi sakdi: " + filename);
            // (Can't read file)
            return http::HttpResponse::internalError("Cannot read file");
        }

        // MIME type detect karo
        // (Detect MIME type)
        std::string mimeType = http::HttpConstants::getMimeType(filename);
        
        // Response banao
        // (Create response)
        http::HttpResponse response(http::HttpConstants::STATUS_OK);
        response.setContentType(mimeType);
        response.setBody(fileData);

        // Text files lai compression apply kar sakde haan
        // (Can apply compression for text files)
        std::string fileDataStr(fileData.begin(), fileData.end());
        applyCompression(response, fileDataStr, request);

        utils::Logger::info("File successfully bhej ditti: " + filename);
        // (File successfully sent)

        return response;
    }

    /**
     * File POST handler - POST /files/{filename}
     * (Handle file upload)
     */
    http::HttpResponse handleFilePost(const http::HttpRequest& request) {
        const std::string& path = request.getPath();
        
        // Filename extract karo
        // (Extract filename)
        std::string filename = path.substr(7);  // 7 = length of "/files/"
        
        utils::Logger::info("File upload request: " + filename);
        
        // Request body file vich save karo
        // (Save request body to file)
        const std::string& body = request.getBody();
        
        if (body.empty()) {
            utils::Logger::warn("Empty body file save karn lai bheji!");
            // (Empty body sent for file save!)
            return http::HttpResponse::badRequest("Empty file content");
        }

        // File write karo
        // (Write file)
        bool success = fileHandler_->writeFile(filename, body);
        
        if (success) {
            utils::Logger::info("File successfully save ho gayi: " + filename + 
                              " (" + std::to_string(body.size()) + " bytes)");
            // (File successfully saved)
            return http::HttpResponse::created("", http::HttpConstants::MIME_TEXT_PLAIN);
        } else {
            utils::Logger::error("File save nahi ho sakdi: " + filename);
            // (File couldn't be saved)
            return http::HttpResponse::internalError("Cannot save file");
        }
    }

    /**
     * Response te compression apply karo agar client support karda hai
     * (Apply compression to response if client supports)
     */
    void applyCompression(http::HttpResponse& response, const std::string& data, 
                         const http::HttpRequest& request) {
        // Accept-Encoding header check karo
        // (Check Accept-Encoding header)
        std::string acceptEncoding = request.getHeader(http::HttpConstants::HEADER_ACCEPT_ENCODING);
        
        if (acceptEncoding.empty()) {
            // Client compression support nahi karda
            // (Client doesn't support compression)
            return;
        }

        // Gzip support hai?
        // (Is gzip supported?)
        if (!compression::GzipCompressor::supportsGzip(acceptEncoding)) {
            return;
        }

        // Compress karn da faida hai?
        // (Is it worth compressing?)
        std::string contentType = request.getHeader(http::HttpConstants::HEADER_CONTENT_TYPE);
        if (contentType.empty()) {
            contentType = http::HttpConstants::MIME_TEXT_PLAIN;
        }
        
        if (!compression::GzipCompressor::shouldCompress(data, contentType)) {
            return;
        }

        // Compress karo!
        // (Compress!)
        auto compressed = compression::GzipCompressor::compress(data);
        
        if (!compressed.empty()) {
            response.setCompressedBody(compressed, http::HttpConstants::ENCODING_GZIP);
            utils::Logger::info("Response gzip compressed bhej ditti!");
            // (Response sent gzip compressed!)
        }
    }
};

} // namespace handlers
