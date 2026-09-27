#include "handlers/routes/head_adapter.hpp"

// head_adapter.cpp - HEAD is GET without the body.

namespace handlers::routes {

RouteHandlerFn asHead(RouteHandlerFn getHandler) {
    return [getHandler = std::move(getHandler)](const RouteContext& ctx) {
        http::HttpResponse response = getHandler(ctx);
        response.setHeadersOnly(true);
        return response;
    };
}

} // namespace handlers::routes
