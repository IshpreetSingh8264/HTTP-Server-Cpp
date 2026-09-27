#pragma once

// HttpRequest.hpp - Parsed HTTP request (protocol value object)
// Client ne ki bhejeya hai, sab samjho!
// (Understand what the client has sent!)

#include <string>
#include <unordered_map>

namespace http {

/**
 * HttpRequest - An immutable-after-parse view of one HTTP request
 *
 * Purpose: Raw bytes nu typed struct vich badal do
 *          (Turn raw bytes into a typed struct)
 *
 * HTTP request format:
 *   GET /path HTTP/1.1\r\n
 *   Header1: Value1\r\n
 *   \r\n
 *   Body (optional)
 *
 * Header lookup is case-insensitive, as the HTTP spec requires. This type
 * knows nothing about routing, sockets or compression.
 */
class HttpRequest {
private:
    std::string method_;                                         // GET, POST, ...
    std::string path_;                                           // /echo/hello
    std::string version_;                                        // HTTP/1.1
    std::unordered_map<std::string, std::string> headers_;       // name -> value
    std::string body_;                                           // Request body
    bool valid_;                                                 // Parse successful?

    bool parseRequestLine(const std::string& line);
    void parseHeaderLine(const std::string& line);

public:
    HttpRequest() : valid_(false) {}

    /// Parse a complete raw request. Returns false (and leaves isValid() false) on garbage.
    bool parse(const std::string& rawRequest);

    const std::string& getMethod() const { return method_; }
    const std::string& getPath() const { return path_; }
    const std::string& getVersion() const { return version_; }
    const std::string& getBody() const { return body_; }
    bool isValid() const { return valid_; }

    /// Case-insensitive header lookup; "" when absent.
    std::string getHeader(const std::string& name) const;

    bool hasHeader(const std::string& name) const;

    const std::unordered_map<std::string, std::string>& getHeaders() const { return headers_; }
};

} // namespace http
