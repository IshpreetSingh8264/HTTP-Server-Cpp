#pragma once

// HttpResponse.hpp - HTTP response builder
// Client nu response bhejna hai - format sahi honi chahidi!
// (Need to send response to client - format must be correct!)

#include <string>
#include <unordered_map>
#include <sstream>
#include "http/HttpConstants.hpp"
#include "utils/Logger.hpp"

namespace http {

/**
 * HttpResponse - Build HTTP response to send to client
 * 
 * Purpose: Response nu proper HTTP format vich banana
 *          (Build response in proper HTTP format)
 * 
 * HTTP Response Format:
 * HTTP/1.1 200 OK\r\n
 * Content-Type: text/plain\r\n
 * Content-Length: 5\r\n
 * \r\n
 * Hello
 */
class HttpResponse {
private:
    int statusCode_;            // 200, 404, 500, etc.
    std::string statusText_;    // "OK", "Not Found", etc.
    std::string version_;       // HTTP/1.1
    std::unordered_map<std::string, std::string> headers_;  // Response headers
    std::string body_;          // Response body
    bool isCompressed_;         // Body compressed hai ya nahi
                               // (Whether body is compressed)

public:
    /**
     * Constructor - Default 200 OK response banao
     * (Constructor - Create default 200 OK response)
     */
    HttpResponse(int statusCode = HttpConstants::STATUS_OK) 
        : statusCode_(statusCode),
          version_(HttpConstants::VERSION_1_1),
          isCompressed_(false) {
        statusText_ = HttpConstants::getStatusText(statusCode);
        
        utils::Logger::debug("Response bana rahe haan: " + std::to_string(statusCode) + " " + statusText_);
        // (Creating response)
    }

    // ==================== Setters ====================
    
    /**
     * Status code set karo
     * (Set status code)
     */
    void setStatus(int code) {
        statusCode_ = code;
        statusText_ = HttpConstants::getStatusText(code);
    }

    /**
     * Header add karo
     * (Add header)
     */
    void setHeader(const std::string& name, const std::string& value) {
        headers_[name] = value;
        utils::Logger::debug("Header add kitti: " + name + " = " + value);
        // (Added header)
    }

    /**
     * Body set karo - string format vich
     * (Set body - in string format)
     */
    void setBody(const std::string& body) {
        body_ = body;
        
        // Content-Length automatic add kar do
        // (Automatically add Content-Length)
        setHeader(HttpConstants::HEADER_CONTENT_LENGTH, std::to_string(body_.length()));
        
        utils::Logger::debug("Body set kitti, size: " + std::to_string(body_.length()) + " bytes");
        // (Set body, size: X bytes)
    }

    /**
     * Body set karo - binary data (vector<char>) format vich
     * (Set body - in binary data format)
     */
    void setBody(const std::vector<char>& data) {
        body_ = std::string(data.begin(), data.end());
        setHeader(HttpConstants::HEADER_CONTENT_LENGTH, std::to_string(body_.length()));
        
        utils::Logger::debug("Binary body set kitti, size: " + std::to_string(body_.length()) + " bytes");
        // (Set binary body)
    }

    /**
     * Compressed body set karo (gzip etc)
     * (Set compressed body)
     */
    void setCompressedBody(const std::vector<char>& compressedData, const std::string& encoding) {
        body_ = std::string(compressedData.begin(), compressedData.end());
        setHeader(HttpConstants::HEADER_CONTENT_LENGTH, std::to_string(body_.length()));
        setHeader(HttpConstants::HEADER_CONTENT_ENCODING, encoding);
        isCompressed_ = true;
        
        utils::Logger::debug("Compressed body set kitti (" + encoding + "), size: " + 
                           std::to_string(body_.length()) + " bytes");
        // (Set compressed body)
    }

    /**
     * Content type set karo
     * (Set content type)
     */
    void setContentType(const std::string& mimeType) {
        setHeader(HttpConstants::HEADER_CONTENT_TYPE, mimeType);
    }

    /**
     * Connection header set karo (keep-alive ya close)
     * (Set connection header)
     */
    void setConnection(const std::string& value) {
        setHeader(HttpConstants::HEADER_CONNECTION, value);
    }

    // ==================== Response Building ====================
    
    /**
     * Complete HTTP response string banao
     * (Build complete HTTP response string)
     * 
     * Returns: Properly formatted HTTP response ready to send
     */
    std::string toString() const {
        std::ostringstream oss;
        
        // Status line: HTTP/1.1 200 OK\r\n
        oss << version_ << " " << statusCode_ << " " << statusText_ << HttpConstants::CRLF;
        
        // Saare headers add karo
        // (Add all headers)
        for (const auto& [name, value] : headers_) {
            oss << name << ": " << value << HttpConstants::CRLF;
        }
        
        // Headers khatam, blank line
        // (End of headers, blank line)
        oss << HttpConstants::CRLF;
        
        // Body add karo (agar hai toh)
        // (Add body if present)
        if (!body_.empty()) {
            oss << body_;
        }
        
        utils::Logger::debug("Response tayar hai, total size: " + std::to_string(oss.str().length()) + " bytes");
        // (Response ready)
        
        return oss.str();
    }

    /**
     * Just headers return karo (HEAD request lai)
     * (Return just headers - for HEAD request)
     */
    std::string toStringHeadersOnly() const {
        std::ostringstream oss;
        
        // Status line
        oss << version_ << " " << statusCode_ << " " << statusText_ << HttpConstants::CRLF;
        
        // Headers
        for (const auto& [name, value] : headers_) {
            oss << name << ": " << value << HttpConstants::CRLF;
        }
        
        // End of headers
        oss << HttpConstants::CRLF;
        
        return oss.str();
    }

    // ==================== Convenience Factory Methods ====================
    
    /**
     * 200 OK response with text body
     * (Create 200 OK response with text body)
     */
    static HttpResponse ok(const std::string& body, const std::string& contentType = HttpConstants::MIME_TEXT_PLAIN) {
        HttpResponse response(HttpConstants::STATUS_OK);
        response.setContentType(contentType);
        response.setBody(body);
        return response;
    }

    /**
     * 201 Created response
     * (Create 201 Created response)
     */
    static HttpResponse created(const std::string& body = "", const std::string& contentType = HttpConstants::MIME_TEXT_PLAIN) {
        HttpResponse response(HttpConstants::STATUS_CREATED);
        if (!body.empty()) {
            response.setContentType(contentType);
            response.setBody(body);
        } else {
            response.setHeader(HttpConstants::HEADER_CONTENT_LENGTH, "0");
        }
        return response;
    }

    /**
     * 404 Not Found response
     * (Create 404 Not Found response)
     */
    static HttpResponse notFound(const std::string& message = "Not Found") {
        HttpResponse response(HttpConstants::STATUS_NOT_FOUND);
        response.setContentType(HttpConstants::MIME_TEXT_PLAIN);
        response.setBody(message);
        
        utils::Logger::warn("404 response bhej rahe haan: " + message);
        // (Sending 404 response)
        
        return response;
    }

    /**
     * 500 Internal Server Error response
     * (Create 500 error response)
     */
    static HttpResponse internalError(const std::string& message = "Internal Server Error") {
        HttpResponse response(HttpConstants::STATUS_INTERNAL_ERROR);
        response.setContentType(HttpConstants::MIME_TEXT_PLAIN);
        response.setBody(message);
        
        utils::Logger::error("500 error bhej rahe haan! Server vich gadbad: " + message);
        // (Sending 500 error! Server problem)
        
        return response;
    }

    /**
     * 400 Bad Request response
     * (Create 400 bad request response)
     */
    static HttpResponse badRequest(const std::string& message = "Bad Request") {
        HttpResponse response(HttpConstants::STATUS_BAD_REQUEST);
        response.setContentType(HttpConstants::MIME_TEXT_PLAIN);
        response.setBody(message);
        
        utils::Logger::warn("400 Bad Request bhej rahe haan: " + message);
        // (Sending 400 Bad Request)
        
        return response;
    }
};

} // namespace http
