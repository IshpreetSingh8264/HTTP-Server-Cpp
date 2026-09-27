#pragma once

// HttpResponse.hpp - HTTP response builder (protocol value object)
// Client nu response bhejna hai - format sahi honi chahidi!
// (Build the response to send to the client - the format must be correct!)

#include <string>
#include <unordered_map>
#include <vector>

#include "http/HttpConstants.hpp"

namespace http {

/**
 * HttpResponse - A response under construction
 *
 * Purpose: Response nu proper HTTP format vich banana
 *          (Build the response in proper HTTP format)
 *
 * HTTP response format:
 *   HTTP/1.1 200 OK\r\n
 *   Content-Type: text/plain\r\n
 *   Content-Length: 5\r\n
 *   \r\n
 *   Hello
 *
 * Setting the body (compressed or not) keeps Content-Length in sync
 * automatically. Serialisation to the wire happens in `toString()`.
 */
class HttpResponse {
private:
    int statusCode_;                                       // 200, 404, 500, ...
    std::string statusText_;                               // "OK", "Not Found", ...
    std::string version_;                                  // HTTP/1.1
    std::unordered_map<std::string, std::string> headers_; // Response headers
    std::string body_;                                     // Response body
    bool isCompressed_;                                    // Body compressed?
    bool headersOnly_;                                     // HEAD: suppress the body

public:
    explicit HttpResponse(int statusCode = HttpConstants::STATUS_OK);

    // ==================== Setters ====================

    void setStatus(int code);
    void setHeader(const std::string& name, const std::string& value);
    void setContentType(const std::string& mimeType);
    void setConnection(const std::string& value);

    /// Set a text body and refresh Content-Length.
    void setBody(const std::string& body);

    /// Set a binary body and refresh Content-Length.
    void setBody(const std::vector<char>& data);

    /// Set a pre-compressed body, refresh Content-Length, tag Content-Encoding.
    void setCompressedBody(const std::vector<char>& compressedData, const std::string& encoding);

    // ==================== Getters ====================

    std::string getHeader(const std::string& name) const;
    const std::string& getBody() const { return body_; }
    bool isCompressed() const { return isCompressed_; }
    int getStatusCode() const { return statusCode_; }

    /// HEAD: serialise the status line and headers but not the body. The body
    /// is still built, so Content-Length stays truthful.
    void setHeadersOnly(bool headersOnly) { headersOnly_ = headersOnly; }
    bool isHeadersOnly() const { return headersOnly_; }

    // ==================== Serialisation ====================

    /// Full response: status line, headers, blank line, body.
    std::string toString() const;

    /// Status line and headers only, no body - the HEAD response.
    std::string toStringHeadersOnly() const;

    // ==================== Convenience Factories ====================

    static HttpResponse ok(const std::string& body,
                           const std::string& contentType = HttpConstants::MIME_TEXT_PLAIN);
    static HttpResponse created(const std::string& body = "",
                                const std::string& contentType = HttpConstants::MIME_TEXT_PLAIN);
    static HttpResponse notFound(const std::string& message = "Not Found");
    static HttpResponse methodNotAllowed(const std::string& message = "Method Not Allowed");
    static HttpResponse internalError(const std::string& message = "Internal Server Error");
    static HttpResponse badRequest(const std::string& message = "Bad Request");
};

} // namespace http
