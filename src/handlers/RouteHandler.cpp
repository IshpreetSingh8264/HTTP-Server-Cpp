#include "handlers/RouteHandler.hpp"

// RouteHandler.cpp - Wiring only. The routing table does the work.

#include "handlers/routes/route_registry.hpp"
#include "utils/Logger.hpp"

namespace handlers {

RouteHandler::RouteHandler(std::shared_ptr<FileHandler> fileHandler)
    : fileHandler_(std::move(fileHandler)) {
    utils::Logger::debug("RouteHandler tayar, " + std::to_string(routes::routeCount()) + " routes");
}

http::HttpResponse RouteHandler::handleRequest(const http::HttpRequest& request) {
    return routes::dispatch(request, fileHandler_);
}

} // namespace handlers
