#pragma once

// head_adapter.hpp - Turn a GET response into its HEAD counterpart
// HEAD nu alag logic chahida hi nahi - sirf body hata do.
// (HEAD needs no logic of its own - just drop the body.)

#include "handlers/routes/route_registry.hpp"

namespace handlers::routes {

/**
 * Wrap a GET handler so its response is serialised as headers only.
 *
 * The body is still produced (and still counted for Content-Length) - it is
 * simply not written to the wire, which is what RFC 9110 requires: a HEAD
 * response carries the headers the equivalent GET would have carried,
 * including the uncompressed/compressed Content-Length, and no body.
 */
RouteHandlerFn asHead(RouteHandlerFn getHandler);

} // namespace handlers::routes
