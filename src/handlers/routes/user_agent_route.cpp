// user_agent_route - endpoint handler

#include "handlers/routes/route_handlers.hpp"
#include "http/HttpConstants.hpp"
#include "utils/Logger.hpp"

namespace handlers::routes {

http::HttpResponse handleUserAgent(const RouteContext& ctx) {
    std::string userAgent = ctx.request.getHeader(http::HttpConstants::HEADER_USER_AGENT);

    if (userAgent.empty()) {
        utils::Logger::warn("User-Agent header nahi mili!");
        userAgent = "Unknown";
    }

    return http::HttpResponse::ok(userAgent, http::HttpConstants::MIME_TEXT_PLAIN);
}

} // namespace handlers::routes
