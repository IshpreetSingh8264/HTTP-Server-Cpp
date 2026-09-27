#include "http/HttpResponse.hpp"

// HttpResponse.cpp - Status line, header emission and body framing.

#include <sstream>

#include "utils/Logger.hpp"

namespace http {

HttpResponse::HttpResponse(int statusCode)
    : statusCode_(statusCode),
      version_(HttpConstants::VERSION_1_1),
      isCompressed_(false) {
    statusText_ = HttpConstants::getStatusText(statusCode);
    utils::Logger::debug("Response bana rahe haan: " + std::to_string(statusCode) + " " + statusText_);
}

void HttpResponse::setStatus(int code) {
    statusCode_ = code;
    statusText_ = HttpConstants::getStatusText(code);
}

void HttpResponse::setHeader(const std::string& name, const std::string& value) {
    headers_[name] = value;
}

void HttpResponse::setContentType(const std::string& mimeType) {
    setHeader(HttpConstants::HEADER_CONTENT_TYPE, mimeType);
}

void HttpResponse::setConnection(const std::string& value) {
    setHeader(HttpConstants::HEADER_CONNECTION, value);
}

void HttpResponse::setBody(const std::string& body) {
    body_ = body;
    setHeader(HttpConstants::HEADER_CONTENT_LENGTH, std::to_string(body_.length()));
}

void HttpResponse::setBody(const std::vector<char>& data) {
    body_.assign(data.begin(), data.end());
    setHeader(HttpConstants::HEADER_CONTENT_LENGTH, std::to_string(body_.length()));
}

void HttpResponse::setCompressedBody(const std::vector<char>& compressedData,
                                     const std::string& encoding) {
    body_.assign(compressedData.begin(), compressedData.end());
    setHeader(HttpConstants::HEADER_CONTENT_LENGTH, std::to_string(body_.length()));
    setHeader(HttpConstants::HEADER_CONTENT_ENCODING, encoding);
    isCompressed_ = true;
    utils::Logger::debug("Compressed body set kitti (" + encoding + "), " +
                         std::to_string(body_.length()) + " bytes");
}

std::string HttpResponse::getHeader(const std::string& name) const {
    auto it = headers_.find(name);
    if (it != headers_.end()) {
        return it->second;
    }
    return "";
}

std::string HttpResponse::toString() const {
    std::ostringstream oss;

    oss << version_ << " " << statusCode_ << " " << statusText_ << HttpConstants::CRLF;

    for (const auto& [name, value] : headers_) {
        oss << name << ": " << value << HttpConstants::CRLF;
    }

    oss << HttpConstants::CRLF;
    oss << body_;

    return oss.str();
}

std::string HttpResponse::toStringHeadersOnly() const {
    std::ostringstream oss;

    oss << version_ << " " << statusCode_ << " " << statusText_ << HttpConstants::CRLF;

    for (const auto& [name, value] : headers_) {
        oss << name << ": " << value << HttpConstants::CRLF;
    }

    oss << HttpConstants::CRLF;
    return oss.str();
}

HttpResponse HttpResponse::ok(const std::string& body, const std::string& contentType) {
    HttpResponse response(HttpConstants::STATUS_OK);
    response.setContentType(contentType);
    response.setBody(body);
    return response;
}

HttpResponse HttpResponse::created(const std::string& body, const std::string& contentType) {
    HttpResponse response(HttpConstants::STATUS_CREATED);
    if (!body.empty()) {
        response.setContentType(contentType);
        response.setBody(body);
    } else {
        response.setHeader(HttpConstants::HEADER_CONTENT_LENGTH, "0");
    }
    return response;
}

HttpResponse HttpResponse::notFound(const std::string& message) {
    HttpResponse response(HttpConstants::STATUS_NOT_FOUND);
    response.setContentType(HttpConstants::MIME_TEXT_PLAIN);
    response.setBody(message);
    utils::Logger::debug("404 response: " + message);
    return response;
}

HttpResponse HttpResponse::methodNotAllowed(const std::string& message) {
    HttpResponse response(HttpConstants::STATUS_METHOD_NOT_ALLOWED);
    response.setContentType(HttpConstants::MIME_TEXT_PLAIN);
    response.setBody(message);
    utils::Logger::debug("405 response: " + message);
    return response;
}

HttpResponse HttpResponse::internalError(const std::string& message) {
    HttpResponse response(HttpConstants::STATUS_INTERNAL_ERROR);
    response.setContentType(HttpConstants::MIME_TEXT_PLAIN);
    response.setBody(message);
    utils::Logger::error("500 error bhej rahe haan! Server vich gadbad: " + message);
    return response;
}

HttpResponse HttpResponse::badRequest(const std::string& message) {
    HttpResponse response(HttpConstants::STATUS_BAD_REQUEST);
    response.setContentType(HttpConstants::MIME_TEXT_PLAIN);
    response.setBody(message);
    utils::Logger::warn("400 Bad Request bhej rahe haan: " + message);
    return response;
}

} // namespace http
