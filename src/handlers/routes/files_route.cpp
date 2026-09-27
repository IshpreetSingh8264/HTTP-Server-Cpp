// files_route - endpoint handler

#include "handlers/routes/route_handlers.hpp"

#include <string>

#include "http/HttpConstants.hpp"
#include "utils/Logger.hpp"

namespace handlers::routes {

namespace {
// "/files/" prefix da length. The pattern guarantees the prefix is present.
constexpr size_t FILES_PREFIX_LENGTH = 7;
} // namespace

http::HttpResponse handleFileGet(const RouteContext& ctx) {
    const std::string filename = ctx.request.getPath().substr(FILES_PREFIX_LENGTH);

    utils::Logger::info("File GET: " + filename);

    if (!ctx.fileHandler->fileExists(filename)) {
        utils::Logger::warn("File nahi mili: " + filename);
        return http::HttpResponse::notFound("File not found: " + filename);
    }

    const std::vector<char> fileData = ctx.fileHandler->readFile(filename);

    if (fileData.empty()) {
        // NOTE: a zero-byte file therefore 500s. Pre-existing behaviour, kept
        // as-is so this commit is a pure restructure.
        utils::Logger::error("File read ho nahi sakdi: " + filename);
        return http::HttpResponse::internalError("Cannot read file");
    }

    http::HttpResponse response(http::HttpConstants::STATUS_OK);
    response.setContentType(http::HttpConstants::getMimeType(filename));
    response.setBody(fileData);

    return response;
}

http::HttpResponse handleFilePost(const RouteContext& ctx) {
    const std::string filename = ctx.request.getPath().substr(FILES_PREFIX_LENGTH);

    utils::Logger::info("File POST: " + filename);

    const std::string& body = ctx.request.getBody();

    if (body.empty()) {
        utils::Logger::warn("Empty body file save karn lai bheji!");
        return http::HttpResponse::badRequest("Empty file content");
    }

    if (!ctx.fileHandler->writeFile(filename, body)) {
        utils::Logger::error("File save nahi ho sakdi: " + filename);
        return http::HttpResponse::internalError("Cannot save file");
    }

    utils::Logger::info("File save ho gayi: " + filename +
                        " (" + std::to_string(body.size()) + " bytes)");
    return http::HttpResponse::created("", http::HttpConstants::MIME_TEXT_PLAIN);
}

} // namespace handlers::routes
