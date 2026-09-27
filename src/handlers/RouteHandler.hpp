#pragma once

// RouteHandler.hpp - URL routing te request handling
// Different URLs lai different responses!
// (Different responses for different URLs!)

#include <memory>
#include <string>

#include "handlers/FileHandler.hpp"
#include "http/HttpRequest.hpp"
#include "http/HttpResponse.hpp"

namespace handlers {

/**
 * RouteHandler - Turn an HttpRequest into an HttpResponse
 *
 * Purpose: URL path de hisaab se appropriate response bhejo
 *          (Send the appropriate response based on the URL path)
 *
 * Supported routes:
 * - GET /              -> 200 OK
 * - GET /echo/{str}    -> Echo back the string
 * - GET /user-agent    -> Return User-Agent header
 * - GET /files/{name}  -> Return file contents
 * - POST /files/{name} -> Save file
 *
 * TODO(item 2): this class still owns an if-chain; it is meant to shrink to a
 * dispatcher over a data-driven route table.
 */
class RouteHandler {
private:
    std::shared_ptr<FileHandler> fileHandler_;  // File operations lai

    http::HttpResponse handleGetRequest(const http::HttpRequest& request);
    http::HttpResponse handlePostRequest(const http::HttpRequest& request);
    http::HttpResponse handleRoot(const http::HttpRequest& request);
    http::HttpResponse handleEcho(const http::HttpRequest& request);
    http::HttpResponse handleUserAgent(const http::HttpRequest& request);
    http::HttpResponse handleFileGet(const http::HttpRequest& request);
    http::HttpResponse handleFilePost(const http::HttpRequest& request);
    void applyCompression(http::HttpResponse& response, const std::string& data,
                          const http::HttpRequest& request);

public:
    explicit RouteHandler(std::shared_ptr<FileHandler> fileHandler);

    /// Handle a request and produce the response. Never throws for bad input.
    http::HttpResponse handleRequest(const http::HttpRequest& request);
};

} // namespace handlers
