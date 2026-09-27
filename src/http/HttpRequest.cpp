#include "http/HttpRequest.hpp"

// HttpRequest.cpp - Request-line and header parsing.

#include <exception>

#include "http/HttpConstants.hpp"
#include "utils/Logger.hpp"
#include "utils/StringUtils.hpp"

namespace http {

bool HttpRequest::parse(const std::string& rawRequest) {
    if (rawRequest.empty()) {
        utils::Logger::error("Oye! Khali request ayi hai, kuch taan bhejo yaar!");
        valid_ = false;
        return false;
    }

    try {
        // Headers te body nu alag karo
        size_t headerEndPos = rawRequest.find(HttpConstants::HEADER_SEPARATOR);
        std::string headerSection;

        if (headerEndPos != std::string::npos) {
            headerSection = rawRequest.substr(0, headerEndPos);
            const size_t bodyStart = headerEndPos + HttpConstants::HEADER_SEPARATOR_LENGTH;
            if (bodyStart < rawRequest.length()) {
                body_ = rawRequest.substr(bodyStart);
            }
        } else {
            // Separator nahi mileya, sara kuch header hi hai
            headerSection = rawRequest;
        }

        auto lines = utils::StringUtils::split(headerSection, HttpConstants::CRLF);

        if (lines.empty() || lines[0].empty()) {
            utils::Logger::error("Request vich koi line nahi! Gadbad hai bhai.");
            valid_ = false;
            return false;
        }

        if (!parseRequestLine(lines[0])) {
            return false;
        }

        for (size_t i = 1; i < lines.size(); ++i) {
            if (!lines[i].empty()) {
                parseHeaderLine(lines[i]);
            }
        }

        valid_ = true;
        utils::Logger::debug("Request parse ho gayi: " + method_ + " " + path_);
        return true;

    } catch (const std::exception& e) {
        utils::Logger::error("Request parsing vich exception: " + std::string(e.what()));
        valid_ = false;
        return false;
    }
}

std::string HttpRequest::getHeader(const std::string& name) const {
    // Header names case-insensitive hunde ne
    for (const auto& [key, value] : headers_) {
        if (utils::StringUtils::equalsIgnoreCase(key, name)) {
            return value;
        }
    }
    return "";
}

bool HttpRequest::hasHeader(const std::string& name) const {
    for (const auto& [key, value] : headers_) {
        if (utils::StringUtils::equalsIgnoreCase(key, name)) {
            return true;
        }
    }
    return false;
}

bool HttpRequest::parseRequestLine(const std::string& line) {
    // "GET /path HTTP/1.1" - exactly three space-separated parts.
    auto parts = utils::StringUtils::split(line, ' ');

    if (parts.size() != 3) {
        utils::Logger::error("Request line galat hai! Teen parts hone chahide: " + line);
        valid_ = false;
        return false;
    }

    method_ = parts[0];
    path_ = parts[1];
    version_ = parts[2];

    if (version_ != HttpConstants::VERSION_1_1 && version_ != HttpConstants::VERSION_1_0) {
        utils::Logger::warn("Oye! HTTP version " + version_ + " jaani-pehchani nahi!");
    }

    return true;
}

void HttpRequest::parseHeaderLine(const std::string& line) {
    const size_t colonPos = line.find(':');

    if (colonPos == std::string::npos) {
        utils::Logger::warn("Header line vich colon nahi! Skip kar ditte: " + line);
        return;
    }

    const std::string name = utils::StringUtils::trim(line.substr(0, colonPos));
    const std::string value = utils::StringUtils::trim(line.substr(colonPos + 1));

    if (name.empty()) {
        utils::Logger::warn("Header name khali hai! Skip kar ditte: " + line);
        return;
    }

    // A field may legitimately arrive more than once. List-valued fields such
    // as Accept-Encoding are then the comma-joined union of every occurrence,
    // not a last-one-wins overwrite - otherwise
    //   curl -H 'Accept-Encoding: deflate' -H 'Accept-Encoding: gzip'
    // would silently lose the deflate offer.
    auto existing = headers_.find(name);
    if (existing != headers_.end()) {
        existing->second += ", " + value;
        return;
    }

    headers_[name] = value;
}

} // namespace http
