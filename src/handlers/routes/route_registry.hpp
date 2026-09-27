#pragma once

// route_registry.hpp - The routing table (Rule 5: data, not control flow)
// Routing ha routing data hai - ek if-chain nahi.
// (Routing is data - not an if-chain.)

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "handlers/FileHandler.hpp"
#include "http/HttpRequest.hpp"
#include "http/HttpResponse.hpp"

namespace handlers::routes {

/**
 * RouteContext - Everything a route handler is allowed to see
 *
 * Handlers receive this one typed struct instead of reaching for globals or
 * back-referencing the dispatcher (Rule 4).
 */
struct RouteContext {
    const http::HttpRequest& request;
    const std::shared_ptr<FileHandler>& fileHandler;
};

/**
 * The one and only route handler signature (Rule 6).
 * Every entry in the table conforms to it, which is what makes a new route a
 * single line of data rather than a new branch.
 */
using RouteHandlerFn = std::function<http::HttpResponse(const RouteContext&)>;

/**
 * One row of the routing table.
 *
 * `pathPattern` is a slash-separated pattern whose segments are either
 * literals or parameters:
 *
 *   /files/{name}     one segment, captured as "name"
 *   /files/{name...}  one or more trailing segments, joined with '/'
 *
 * Parameters are positional. The handler that wants a parameter re-derives it
 * from request.getPath(), which keeps the matcher free of string building and
 * the table free of per-route plumbing.
 */
struct Route {
    std::string method;       // "GET", "POST", ...
    std::string pathPattern;  // "/", "/echo/{str}", ...
    RouteHandlerFn handler;
};

/// The table, built once on first use and never mutated afterwards.
const std::vector<Route>& routeTable();

/// Number of rows in the table (diagnostics and tests).
std::size_t routeCount();

/**
 * Match one request against the table and invoke the winning handler.
 *
 * Resolution order:
 *   1. method + path match            -> call the handler
 *   2. path matches, method does not  -> 405 Method Not Allowed
 *   3. nothing matches                -> 404 Not Found
 *
 * Also applies the response-compression policy, because that is a
 * cross-cutting concern every route shares and no route should have to
 * remember (Rule 8).
 */
http::HttpResponse dispatch(const http::HttpRequest& request,
                            const std::shared_ptr<FileHandler>& fileHandler);

/// True when `path` matches `pattern`. Exposed for tests.
bool pathMatches(const std::string& path, const std::string& pattern);

} // namespace handlers::routes
