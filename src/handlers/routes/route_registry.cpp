#include "handlers/routes/route_registry.hpp"

// route_registry.cpp - The routing table, the pattern matcher and the dispatcher.
//
// This file is the whole of the routing mechanism. There is no if-chain on
// method and no if-chain on path: adding an endpoint means adding one row to
// kRouteTable and one function in a sibling file.

#include <string>
#include <vector>

#include "handlers/routes/head_adapter.hpp"
#include "handlers/routes/route_handlers.hpp"
#include "compression/GzipCompressor.hpp"
#include "http/HttpConstants.hpp"
#include "utils/Logger.hpp"
#include "utils/StringUtils.hpp"

namespace handlers::routes {

namespace {

/**
 * The routing table (Rule 5).
 *
 * Order is irrelevant to correctness - the matcher is exact - but it is kept
 * in a readable order: fixed paths first, then prefix routes, then files.
 */
const std::vector<Route>& table() {
    static const std::vector<Route> kRouteTable = {
        {http::HttpConstants::METHOD_GET,    "/",                  &handleRoot},
        {http::HttpConstants::METHOD_GET,    "/echo/{str}",        &handleEcho},
        {http::HttpConstants::METHOD_GET,    "/user-agent",        &handleUserAgent},
        {http::HttpConstants::METHOD_GET,    "/files/{name...}",   &handleFileGet},
        {http::HttpConstants::METHOD_POST,   "/files/{name...}",   &handleFilePost},

        // HEAD rows wrap the GET rows; see head_adapter.hpp.
        {http::HttpConstants::METHOD_HEAD,   "/",                  asHead(&handleRoot)},
        {http::HttpConstants::METHOD_HEAD,   "/echo/{str}",        asHead(&handleEcho)},
        {http::HttpConstants::METHOD_HEAD,   "/user-agent",        asHead(&handleUserAgent)},
        {http::HttpConstants::METHOD_HEAD,   "/files/{name...}",   asHead(&handleFileGet)},
    };
    return kRouteTable;
}

} // namespace

const std::vector<Route>& routeTable() {
    return table();
}

std::size_t routeCount() {
    return table().size();
}

namespace {

// The "..." marker that turns a parameter into a greedy tail.
constexpr const char* TAIL_SUFFIX = "...";
// sizeof on the array literal, not on TAIL_SUFFIX - which is a pointer and
// would yield 8.
constexpr size_t TAIL_SUFFIX_LENGTH = sizeof("...") - 1;
static_assert(TAIL_SUFFIX_LENGTH == 3, "unexpected tail suffix length");

/// "{name}" - a positional parameter segment.
bool isParameter(const std::string& segment) {
    return segment.size() > 2 && segment.front() == '{' && segment.back() == '}';
}

/// "{name...}" - a parameter that also swallows the remaining path segments.
/// The "..." marker lives inside the braces, so it ends the *name*, not the
/// whole segment.
bool isGreedyParameter(const std::string& segment) {
    if (!isParameter(segment)) {
        return false;
    }
    const std::string name = segment.substr(1, segment.size() - 2);
    return name.size() > TAIL_SUFFIX_LENGTH && utils::StringUtils::endsWith(name, TAIL_SUFFIX);
}

} // namespace

bool pathMatches(const std::string& path, const std::string& pattern) {
    const auto pathParts = utils::StringUtils::split(path, '/');
    const auto patternParts = utils::StringUtils::split(pattern, '/');

    size_t i = 0;
    for (; i < patternParts.size(); ++i) {
        const std::string& segment = patternParts[i];

        // Greedy tail: consume every remaining path segment, so the pattern
        // must be no longer than the path.
        if (isGreedyParameter(segment)) {
            return i < pathParts.size();
        }

        if (i >= pathParts.size()) {
            return false;
        }

        // A bare "{name}" matches exactly one path segment.
        if (!isParameter(segment) && segment != pathParts[i]) {
            return false;
        }
    }

    return i == pathParts.size();
}

namespace {

/**
 * Apply the shared response-compression policy.
 *
 * This lives in the dispatcher rather than in each route on purpose: whether a
 * body may be compressed depends on the request and on the response, not on
 * which endpoint produced it, so no handler has to remember (Rule 8).
 */
void applyContentEncoding(http::HttpResponse& response, const http::HttpRequest& request) {
    const std::string acceptEncoding =
        request.getHeader(http::HttpConstants::HEADER_ACCEPT_ENCODING);

    if (acceptEncoding.empty()) {
        return;
    }
    if (!compression::GzipCompressor::supportsGzip(acceptEncoding)) {
        return;
    }

    // The response's Content-Type decides, not the request's.
    std::string contentType = response.getHeader(http::HttpConstants::HEADER_CONTENT_TYPE);
    if (contentType.empty()) {
        contentType = http::HttpConstants::MIME_TEXT_PLAIN;
    }

    if (!compression::GzipCompressor::shouldCompress(response.getBody(), contentType)) {
        return;
    }

    const std::vector<char> compressed =
        compression::GzipCompressor::compress(response.getBody());

    if (!compressed.empty()) {
        response.setCompressedBody(compressed, http::HttpConstants::ENCODING_GZIP);
        utils::Logger::debug("Response gzip compressed: " + std::to_string(compressed.size()) +
                             " bytes");
    }
}

} // namespace

http::HttpResponse dispatch(const http::HttpRequest& request,
                            const std::shared_ptr<FileHandler>& fileHandler) {
    const std::string& method = request.getMethod();
    const std::string& path = request.getPath();

    utils::Logger::debug("Dispatch: " + method + " " + path);

    bool pathMatchedSomeRoute = false;

    for (const Route& route : table()) {
        if (!pathMatches(path, route.pathPattern)) {
            continue;
        }
        pathMatchedSomeRoute = true;

        if (method != route.method) {
            continue;
        }

        const RouteContext ctx{request, fileHandler};
        http::HttpResponse response = route.handler(ctx);

        // Compress before serialisation so a HEAD response reports the
        // Content-Length its GET counterpart would have reported.
        applyContentEncoding(response, request);
        return response;
    }

    if (pathMatchedSomeRoute) {
        utils::Logger::debug("405: " + method + " " + path);
        return http::HttpResponse::methodNotAllowed(method + " is not allowed for " + path);
    }

    utils::Logger::debug("404: " + path);
    return http::HttpResponse::notFound("Path not found: " + path);
}

} // namespace handlers::routes
