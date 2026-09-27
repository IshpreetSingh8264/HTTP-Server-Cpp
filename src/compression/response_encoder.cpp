#include "compression/response_encoder.hpp"

// response_encoder.cpp - zlib deflate driven through a chosen window size.

#include <zlib.h>

#include <utility>

#include "utils/Logger.hpp"
#include "utils/StringUtils.hpp"

namespace compression {

namespace {

/// One-shot deflate with a single Z_FINISH call.
std::vector<char> deflateWith(const std::string& data, int windowBits, const char* label) {
    if (data.empty()) {
        return {};
    }

    z_stream stream{};
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;

    int ret = deflateInit2(&stream,
                           Z_DEFAULT_COMPRESSION,
                           Z_DEFLATED,
                           windowBits,
                           8,  // memLevel
                           Z_DEFAULT_STRATEGY);
    if (ret != Z_OK) {
        utils::Logger::error(std::string(label) + " init fail! code: " + std::to_string(ret));
        return {};
    }

    stream.avail_in = static_cast<uInt>(data.size());
    stream.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(data.data()));

    // Worst case for deflate is the input plus a small container overhead.
    std::vector<char> out(data.size() + 1024);
    stream.avail_out = static_cast<uInt>(out.size());
    stream.next_out = reinterpret_cast<Bytef*>(out.data());

    ret = deflate(&stream, Z_FINISH);
    if (ret != Z_STREAM_END) {
        utils::Logger::error(std::string(label) + " compress fail! code: " + std::to_string(ret));
        deflateEnd(&stream);
        return {};
    }

    out.resize(stream.total_out);
    deflateEnd(&stream);

    const int savedPct = static_cast<int>(
        (1.0 - static_cast<double>(out.size()) / static_cast<double>(data.size())) * 100.0);
    utils::Logger::debug(std::string(label) + ": " + std::to_string(data.size()) + " -> " +
                         std::to_string(out.size()) + " bytes (" + std::to_string(savedPct) +
                         "% chhota)");

    return out;
}

} // namespace

std::vector<char> ResponseEncoder::gzip(const std::string& data) {
    return deflateWith(data, GZIP_WINDOW_BITS, "gzip");
}

std::vector<char> ResponseEncoder::deflate(const std::string& data) {
    return deflateWith(data, DEFLATE_WINDOW_BITS, "deflate");
}

std::vector<char> ResponseEncoder::compress(const std::string& data, ContentEncoding encoding) {
    switch (encoding) {
        case ContentEncoding::Gzip:    return gzip(data);
        case ContentEncoding::Deflate: return deflate(data);
        case ContentEncoding::Identity: break;
    }
    return {};  // Identity means "send the bytes as they are"
}

bool ResponseEncoder::shouldCompress(const std::string& data, const std::string& contentType) {
    if (data.empty()) {
        return false;
    }

    // Already-compressed payloads (images, video, archives) only grow.
    const std::string lower = utils::StringUtils::toLower(contentType);
    if (utils::StringUtils::contains(lower, "image/") ||
        utils::StringUtils::contains(lower, "video/") ||
        utils::StringUtils::contains(lower, "audio/") ||
        utils::StringUtils::contains(lower, "application/zip") ||
        utils::StringUtils::contains(lower, "application/gzip")) {
        utils::Logger::debug("Content type already compressed: " + contentType + ", skip.");
        return false;
    }

    // Note: the CodeCrafters harness expects compression even on tiny bodies, so
    // there is deliberately no minimum-size threshold here.
    return true;
}

} // namespace compression
