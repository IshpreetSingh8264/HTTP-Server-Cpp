#pragma once

// HttpConstants.hpp - HTTP protocol constants
// Saare HTTP status codes, methods, MIME types ikko jagah!
// (All HTTP status codes, methods, MIME types in one place!)

#include <string>

namespace http {

/**
 * HttpConstants - Compile-time HTTP protocol vocabulary
 *
 * Purpose: Standard HTTP values nu centralize karna
 *          (Centralize standard HTTP values)
 *
 * Zero state, zero allocation. Every member is `constexpr` except the two
 * lookup helpers, which live in HttpConstants.cpp.
 */
class HttpConstants {
public:
    // ==================== HTTP Methods ====================
    static constexpr const char* METHOD_GET = "GET";
    static constexpr const char* METHOD_POST = "POST";
    static constexpr const char* METHOD_PUT = "PUT";
    static constexpr const char* METHOD_DELETE = "DELETE";
    static constexpr const char* METHOD_HEAD = "HEAD";
    static constexpr const char* METHOD_OPTIONS = "OPTIONS";

    // ==================== HTTP Versions ====================
    static constexpr const char* VERSION_1_0 = "HTTP/1.0";
    static constexpr const char* VERSION_1_1 = "HTTP/1.1";

    // ==================== Status Codes ====================
    // 2xx - Sab kuch sahi gaya! (Everything went well)
    static constexpr int STATUS_OK = 200;
    static constexpr const char* STATUS_OK_TEXT = "OK";

    static constexpr int STATUS_CREATED = 201;
    static constexpr const char* STATUS_CREATED_TEXT = "Created";

    // 4xx - Tussi kuch galat bhejeya (You sent something wrong)
    static constexpr int STATUS_BAD_REQUEST = 400;
    static constexpr const char* STATUS_BAD_REQUEST_TEXT = "Bad Request";

    static constexpr int STATUS_NOT_FOUND = 404;
    static constexpr const char* STATUS_NOT_FOUND_TEXT = "Not Found";

    static constexpr int STATUS_METHOD_NOT_ALLOWED = 405;
    static constexpr const char* STATUS_METHOD_NOT_ALLOWED_TEXT = "Method Not Allowed";

    // 5xx - Assi kuch gadbad kar ditti (We messed up)
    static constexpr int STATUS_INTERNAL_ERROR = 500;
    static constexpr const char* STATUS_INTERNAL_ERROR_TEXT = "Internal Server Error";

    static constexpr int STATUS_NOT_IMPLEMENTED = 501;
    static constexpr const char* STATUS_NOT_IMPLEMENTED_TEXT = "Not Implemented";

    // ==================== Common Headers ====================
    static constexpr const char* HEADER_CONTENT_TYPE = "Content-Type";
    static constexpr const char* HEADER_CONTENT_LENGTH = "Content-Length";
    static constexpr const char* HEADER_CONTENT_ENCODING = "Content-Encoding";
    static constexpr const char* HEADER_ACCEPT_ENCODING = "Accept-Encoding";
    static constexpr const char* HEADER_CONNECTION = "Connection";
    static constexpr const char* HEADER_USER_AGENT = "User-Agent";
    static constexpr const char* HEADER_HOST = "Host";
    static constexpr const char* HEADER_TRANSFER_ENCODING = "Transfer-Encoding";

    // ==================== MIME Types ====================
    static constexpr const char* MIME_TEXT_PLAIN = "text/plain";
    static constexpr const char* MIME_TEXT_HTML = "text/html";
    static constexpr const char* MIME_TEXT_CSS = "text/css";
    static constexpr const char* MIME_TEXT_JAVASCRIPT = "text/javascript";
    static constexpr const char* MIME_APPLICATION_JSON = "application/json";
    static constexpr const char* MIME_APPLICATION_OCTET_STREAM = "application/octet-stream";
    static constexpr const char* MIME_IMAGE_PNG = "image/png";
    static constexpr const char* MIME_IMAGE_JPEG = "image/jpeg";
    static constexpr const char* MIME_IMAGE_GIF = "image/gif";

    // ==================== Connection Values ====================
    static constexpr const char* CONNECTION_KEEP_ALIVE = "keep-alive";
    static constexpr const char* CONNECTION_CLOSE = "close";

    // ==================== Encoding Values ====================
    // NOTE: the literals the server actually emits live in
    // compression::toHeaderValue() (src/compression/content_encoding.hpp).
    // These constants are kept as the protocol-level vocabulary; prefer the
    // compression layer in new code so the two cannot drift apart.
    static constexpr const char* ENCODING_GZIP = "gzip";
    static constexpr const char* ENCODING_DEFLATE = "deflate";
    static constexpr const char* ENCODING_IDENTITY = "identity";

    // ==================== Protocol Constants ====================
    static constexpr const char* CRLF = "\r\n";
    static constexpr const char* HEADER_SEPARATOR = "\r\n\r\n";
    static constexpr size_t HEADER_SEPARATOR_LENGTH = 4;  // sizeof("\r\n\r\n") - 1

    /// Reason phrase for a status code, e.g. 200 -> "OK".
    static std::string getStatusText(int statusCode);

    /// MIME type from a file extension, e.g. "a.html" -> "text/html".
    static std::string getMimeType(const std::string& filename);
};

} // namespace http
