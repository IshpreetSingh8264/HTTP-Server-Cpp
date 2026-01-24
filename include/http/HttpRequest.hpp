#pragma once

// HttpRequest.hpp - HTTP request parser
// Client ne ki bhejeya hai, sab samjho!
// (Understand what client has sent!)

#include <string>
#include <unordered_map>
#include <vector>
#include "utils/StringUtils.hpp"
#include "utils/Logger.hpp"
#include "http/HttpConstants.hpp"

namespace http {

/**
 * HttpRequest - Represents an HTTP request from client
 * 
 * Purpose: Client request nu parse karke understandable form vich rakho
 *          (Parse client request and keep it in understandable form)
 * 
 * HTTP Request Format:
 * GET /path HTTP/1.1\r\n
 * Header1: Value1\r\n
 * Header2: Value2\r\n
 * \r\n
 * Body (optional)
 */
class HttpRequest {
private:
    std::string method_;        // GET, POST, etc.
    std::string path_;          // /echo/hello
    std::string version_;       // HTTP/1.1
    std::unordered_map<std::string, std::string> headers_;  // Header name -> value
    std::string body_;          // Request body (for POST)
    bool valid_;                // Request sahi parse hoyi ya nahi
                               // (Whether request parsed correctly)

public:
    HttpRequest() : valid_(false) {}

    /**
     * Raw HTTP request string nu parse karo
     * (Parse raw HTTP request string)
     * 
     * @param rawRequest: Complete HTTP request as string
     * @return: true if parsing successful, false otherwise
     */
    bool parse(const std::string& rawRequest) {
        if (rawRequest.empty()) {
            utils::Logger::error("Oye! Khali request ayi hai, kuch taan bhejo yaar!");
            // (Hey! Empty request received, send something at least!)
            valid_ = false;
            return false;
        }

        try {
            // Headers te body nu alag karo
            // (Separate headers and body)
            size_t headerEndPos = rawRequest.find(HttpConstants::HEADER_SEPARATOR);
            std::string headerSection;
            
            if (headerEndPos != std::string::npos) {
                headerSection = rawRequest.substr(0, headerEndPos);
                // Body milegi separator ke baad
                // (Body comes after separator)
                size_t bodyStart = headerEndPos + std::string(HttpConstants::HEADER_SEPARATOR).length();
                if (bodyStart < rawRequest.length()) {
                    body_ = rawRequest.substr(bodyStart);
                }
            } else {
                // Separator nahi mileya, sara kuch header hi hai
                // (No separator found, everything is header)
                headerSection = rawRequest;
            }

            // Header section nu lines vich tod do
            // (Break header section into lines)
            auto lines = utils::StringUtils::split(headerSection, "\r\n");
            
            if (lines.empty()) {
                utils::Logger::error("Request vich koi line nahi! Gadbad hai bhai.");
                // (No lines in request! Something's fishy bro.)
                valid_ = false;
                return false;
            }

            // Pehli line - Request line (METHOD PATH VERSION)
            // (First line - Request line)
            if (!parseRequestLine(lines[0])) {
                return false;
            }

            // Baki lines - Headers parse karo
            // (Remaining lines - parse headers)
            for (size_t i = 1; i < lines.size(); ++i) {
                if (!lines[i].empty()) {
                    parseHeaderLine(lines[i]);
                }
            }

            valid_ = true;
            utils::Logger::debug("Request successfully parse ho gayi: " + method_ + " " + path_);
            // (Request successfully parsed)
            return true;

        } catch (const std::exception& e) {
            utils::Logger::error("Request parsing vich exception aa gayi! " + std::string(e.what()));
            // (Exception occurred during request parsing!)
            valid_ = false;
            return false;
        }
    }

    // ==================== Getters ====================
    
    const std::string& getMethod() const { return method_; }
    const std::string& getPath() const { return path_; }
    const std::string& getVersion() const { return version_; }
    const std::string& getBody() const { return body_; }
    bool isValid() const { return valid_; }

    /**
     * Header value milega name se
     * (Get header value by name)
     * 
     * Case-insensitive hai - "Content-Type" == "content-type"
     */
    std::string getHeader(const std::string& name) const {
        // Header names case-insensitive hunde ne
        // (Header names are case-insensitive)
        for (const auto& [key, value] : headers_) {
            if (utils::StringUtils::equalsIgnoreCase(key, name)) {
                return value;
            }
        }
        return ""; // Nahi mileya toh khali string
                  // (Not found, return empty string)
    }

    /**
     * Check karo ki header present hai ya nahi
     * (Check if header is present)
     */
    bool hasHeader(const std::string& name) const {
        for (const auto& [key, value] : headers_) {
            if (utils::StringUtils::equalsIgnoreCase(key, name)) {
                return true;
            }
        }
        return false;
    }

    /**
     * Saare headers milenge
     * (Get all headers)
     */
    const std::unordered_map<std::string, std::string>& getHeaders() const {
        return headers_;
    }

private:
    /**
     * Request line parse karo: "GET /path HTTP/1.1"
     * (Parse request line)
     */
    bool parseRequestLine(const std::string& line) {
        // Space se split karo - 3 parts hone chahide
        // (Split by space - should be 3 parts)
        auto parts = utils::StringUtils::split(line, ' ');
        
        if (parts.size() != 3) {
            utils::Logger::error("Request line galat hai! Teen parts hone chahide: " + line);
            // (Request line is wrong! Should have three parts)
            valid_ = false;
            return false;
        }

        method_ = parts[0];
        path_ = parts[1];
        version_ = parts[2];

        // Version check - HTTP/1.1 ya HTTP/1.0 hona chahida
        // (Version check - should be HTTP/1.1 or HTTP/1.0)
        if (version_ != HttpConstants::VERSION_1_1 && version_ != HttpConstants::VERSION_1_0) {
            utils::Logger::warn("Oye! HTTP version " + version_ + " jaani-pehchani nahi!");
            // (Hey! HTTP version is not recognized!)
        }

        return true;
    }

    /**
     * Header line parse karo: "Content-Type: text/plain"
     * (Parse header line)
     */
    void parseHeaderLine(const std::string& line) {
        // Colon se split karo - name te value milega
        // (Split by colon - get name and value)
        size_t colonPos = line.find(':');
        
        if (colonPos == std::string::npos) {
            utils::Logger::warn("Header line vich colon nahi! Skip kar ditte: " + line);
            // (No colon in header line! Skipping)
            return;
        }

        std::string name = utils::StringUtils::trim(line.substr(0, colonPos));
        std::string value = utils::StringUtils::trim(line.substr(colonPos + 1));

        headers_[name] = value;
    }
};

} // namespace http
