// echo_route - endpoint handler

#include "handlers/routes/route_handlers.hpp"
#include "http/HttpConstants.hpp"
#include "utils/Logger.hpp"

namespace handlers::routes {

namespace {
// "/echo/" prefix da length. The pattern guarantees the prefix is present.
constexpr size_t ECHO_PREFIX_LENGTH = 6;
} // namespace

http::HttpResponse handleEcho(const RouteContext& ctx) {
    const std::string& path = ctx.request.getPath();
    const std::string echoStr = path.substr(ECHO_PREFIX_LENGTH);

    utils::Logger::debug("Echo request: '" + echoStr + "'");
    return http::HttpResponse::ok(echoStr, http::HttpConstants::MIME_TEXT_PLAIN);
}

} // namespace handlers::routes
