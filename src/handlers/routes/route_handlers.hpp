#pragma once

// route_handlers.hpp - One declaration per endpoint handler
// Har endpoint da apna function - koi shared state nahi.
// (One function per endpoint - no shared state.)

#include "handlers/routes/route_registry.hpp"

#include "http/HttpResponse.hpp"

namespace handlers::routes {

// GET /
http::HttpResponse handleRoot(const RouteContext& ctx);

// GET /echo/{str}
http::HttpResponse handleEcho(const RouteContext& ctx);

// GET /user-agent
http::HttpResponse handleUserAgent(const RouteContext& ctx);

// GET /files/{name}
http::HttpResponse handleFileGet(const RouteContext& ctx);

// POST /files/{name}
http::HttpResponse handleFilePost(const RouteContext& ctx);

} // namespace handlers::routes
