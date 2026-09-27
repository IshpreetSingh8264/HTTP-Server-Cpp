// root_route - endpoint handler

#include "handlers/routes/route_handlers.hpp"
#include "http/HttpConstants.hpp"

namespace handlers::routes {

http::HttpResponse handleRoot(const RouteContext& /*ctx*/) {
    // The CodeCrafters stage wants a bare 200. An empty body still carries
    // "Content-Length: 0", which is what a client needs to frame the response.
    return http::HttpResponse::ok("", http::HttpConstants::MIME_TEXT_PLAIN);
}

} // namespace handlers::routes
