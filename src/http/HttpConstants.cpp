#include "http/HttpConstants.hpp"

// HttpConstants.cpp - Status-text and MIME-type lookup tables.

#include <unordered_map>

namespace http {

std::string HttpConstants::getStatusText(int statusCode) {
    switch (statusCode) {
        case STATUS_OK:                 return STATUS_OK_TEXT;
        case STATUS_CREATED:            return STATUS_CREATED_TEXT;
        case STATUS_BAD_REQUEST:        return STATUS_BAD_REQUEST_TEXT;
        case STATUS_NOT_FOUND:          return STATUS_NOT_FOUND_TEXT;
        case STATUS_METHOD_NOT_ALLOWED: return STATUS_METHOD_NOT_ALLOWED_TEXT;
        case STATUS_INTERNAL_ERROR:     return STATUS_INTERNAL_ERROR_TEXT;
        case STATUS_NOT_IMPLEMENTED:    return STATUS_NOT_IMPLEMENTED_TEXT;
        default:                        return "Unknown";
    }
}

std::string HttpConstants::getMimeType(const std::string& filename) {
    const size_t dotPos = filename.find_last_of('.');
    if (dotPos == std::string::npos) {
        return MIME_APPLICATION_OCTET_STREAM;
    }

    std::string ext = filename.substr(dotPos + 1);

    // Function-local static: initialised once, thread-safe under C++11 and later.
    static const std::unordered_map<std::string, std::string> mimeMap = {
        {"html", MIME_TEXT_HTML},
        {"htm",  MIME_TEXT_HTML},
        {"css",  MIME_TEXT_CSS},
        {"js",   MIME_TEXT_JAVASCRIPT},
        {"json", MIME_APPLICATION_JSON},
        {"txt",  MIME_TEXT_PLAIN},
        {"png",  MIME_IMAGE_PNG},
        {"jpg",  MIME_IMAGE_JPEG},
        {"jpeg", MIME_IMAGE_JPEG},
        {"gif",  MIME_IMAGE_GIF}
    };

    auto it = mimeMap.find(ext);
    if (it != mimeMap.end()) {
        return it->second;
    }
    return MIME_APPLICATION_OCTET_STREAM;
}

} // namespace http
