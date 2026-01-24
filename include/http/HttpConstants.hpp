#pragma once

// HttpConstants.hpp - HTTP protocol constants te definitions
// Saare HTTP status codes, methods, MIME types ikko jagah!
// (All HTTP status codes, methods, MIME types in one place!)

#include <string>
#include <unordered_map>

namespace http {

/**
 * HttpConstants - HTTP protocol de constants
 * (HTTP protocol constants)
 * 
 * Purpose: Standard HTTP values nu centralize karna
 *          (Centralize standard HTTP values)
 */
class HttpConstants {
public:
    // ==================== HTTP Methods ====================
    // Koi si request type - GET, POST, PUT, etc.
    // (Any request type - GET, POST, PUT, etc.)
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
    // Response codes - client nu batao ki ki hoya
    // (Response codes - tell client what happened)
    
    // 2xx - Success, sab kuch sahi gaya!
    // (2xx - Success, everything went well!)
    static constexpr int STATUS_OK = 200;
    static constexpr const char* STATUS_OK_TEXT = "OK";
    
    static constexpr int STATUS_CREATED = 201;
    static constexpr const char* STATUS_CREATED_TEXT = "Created";
    
    // 4xx - Client error, tussi kuch galat bhejeya
    // (4xx - Client error, you sent something wrong)
    static constexpr int STATUS_BAD_REQUEST = 400;
    static constexpr const char* STATUS_BAD_REQUEST_TEXT = "Bad Request";
    
    static constexpr int STATUS_NOT_FOUND = 404;
    static constexpr const char* STATUS_NOT_FOUND_TEXT = "Not Found";
    
    static constexpr int STATUS_METHOD_NOT_ALLOWED = 405;
    static constexpr const char* STATUS_METHOD_NOT_ALLOWED_TEXT = "Method Not Allowed";
    
    // 5xx - Server error, assi kuch gadbad kar ditti
    // (5xx - Server error, we messed up something)
    static constexpr int STATUS_INTERNAL_ERROR = 500;
    static constexpr const char* STATUS_INTERNAL_ERROR_TEXT = "Internal Server Error";
    
    static constexpr int STATUS_NOT_IMPLEMENTED = 501;
    static constexpr const char* STATUS_NOT_IMPLEMENTED_TEXT = "Not Implemented";

    // ==================== Common Headers ====================
    // HTTP headers - request te response vich metadata
    // (HTTP headers - metadata in request and response)
    
    static constexpr const char* HEADER_CONTENT_TYPE = "Content-Type";
    static constexpr const char* HEADER_CONTENT_LENGTH = "Content-Length";
    static constexpr const char* HEADER_CONTENT_ENCODING = "Content-Encoding";
    static constexpr const char* HEADER_ACCEPT_ENCODING = "Accept-Encoding";
    static constexpr const char* HEADER_CONNECTION = "Connection";
    static constexpr const char* HEADER_USER_AGENT = "User-Agent";
    static constexpr const char* HEADER_HOST = "Host";
    static constexpr const char* HEADER_TRANSFER_ENCODING = "Transfer-Encoding";

    // ==================== MIME Types ====================
    // File types - client nu batao ki koi format hai data
    // (File types - tell client what format is the data)
    
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
    static constexpr const char* ENCODING_GZIP = "gzip";
    static constexpr const char* ENCODING_DEFLATE = "deflate";
    static constexpr const char* ENCODING_IDENTITY = "identity";

    // ==================== Protocol Constants ====================
    static constexpr const char* CRLF = "\r\n";        // Line ending
    static constexpr const char* HEADER_SEPARATOR = "\r\n\r\n";  // Headers te body vichkar
                                                                  // (Between headers and body)

    /**
     * Status code lai text milega
     * (Get text for status code)
     * 
     * Example: 200 -> "OK", 404 -> "Not Found"
     */
    static std::string getStatusText(int statusCode) {
        // Status code de hisaab se appropriate text return karo
        // (Return appropriate text based on status code)
        switch (statusCode) {
            case STATUS_OK: return STATUS_OK_TEXT;
            case STATUS_CREATED: return STATUS_CREATED_TEXT;
            case STATUS_BAD_REQUEST: return STATUS_BAD_REQUEST_TEXT;
            case STATUS_NOT_FOUND: return STATUS_NOT_FOUND_TEXT;
            case STATUS_METHOD_NOT_ALLOWED: return STATUS_METHOD_NOT_ALLOWED_TEXT;
            case STATUS_INTERNAL_ERROR: return STATUS_INTERNAL_ERROR_TEXT;
            case STATUS_NOT_IMPLEMENTED: return STATUS_NOT_IMPLEMENTED_TEXT;
            default: return "Unknown";
        }
    }

    /**
     * File extension se MIME type guess karo
     * (Guess MIME type from file extension)
     * 
     * Example: "file.html" -> "text/html"
     */
    static std::string getMimeType(const std::string& filename) {
        // Extension dhundho file vich
        // (Find extension in file)
        size_t dotPos = filename.find_last_of('.');
        if (dotPos == std::string::npos) {
            // Extension nahi hai toh default return karo
            // (No extension, return default)
            return MIME_APPLICATION_OCTET_STREAM;
        }
        
        std::string ext = filename.substr(dotPos + 1);
        
        // Extension de hisaab se MIME type return karo
        // (Return MIME type based on extension)
        static const std::unordered_map<std::string, std::string> mimeMap = {
            {"html", MIME_TEXT_HTML},
            {"htm", MIME_TEXT_HTML},
            {"css", MIME_TEXT_CSS},
            {"js", MIME_TEXT_JAVASCRIPT},
            {"json", MIME_APPLICATION_JSON},
            {"txt", MIME_TEXT_PLAIN},
            {"png", MIME_IMAGE_PNG},
            {"jpg", MIME_IMAGE_JPEG},
            {"jpeg", MIME_IMAGE_JPEG},
            {"gif", MIME_IMAGE_GIF}
        };
        
        auto it = mimeMap.find(ext);
        if (it != mimeMap.end()) {
            return it->second;
        }
        
        // Pata nahi ki extension hai toh generic binary
        // (Unknown extension, use generic binary)
        return MIME_APPLICATION_OCTET_STREAM;
    }
};

} // namespace http
