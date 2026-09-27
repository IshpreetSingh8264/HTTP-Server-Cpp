#pragma once

// RouteHandler.hpp - The HTTP dispatcher
// Route table de upar request chalao - apna koi routing logic nahi.
// (Run the request through the route table - no routing logic of its own.)

#include <memory>

#include "handlers/FileHandler.hpp"
#include "http/HttpRequest.hpp"
#include "http/HttpResponse.hpp"

namespace handlers {

/**
 * RouteHandler - Turns an HttpRequest into an HttpResponse
 *
 * Purpose: Hold the request's dependencies and hand off to the route table
 *          (Hold the request's dependencies and hand off to the route table)
 *
 * All routing decisions live in handlers/routes/route_registry.cpp. This class
 * exists so ConnectionHandler depends on one interface rather than on a
 * std::function plus a FileHandler.
 */
class RouteHandler {
private:
    std::shared_ptr<FileHandler> fileHandler_;  // File operations lai

public:
    explicit RouteHandler(std::shared_ptr<FileHandler> fileHandler);

    /// Dispatch one request. Never throws for malformed input.
    http::HttpResponse handleRequest(const http::HttpRequest& request);
};

} // namespace handlers
