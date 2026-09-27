#include "compression/GzipCompressor.hpp"

// GzipCompressor.cpp - zlib deflate wrapped in the gzip container.

#include <zlib.h>

#include "utils/Logger.hpp"
#include "utils/StringUtils.hpp"

namespace compression {

std::vector<char> GzipCompressor::compress(const std::string& data) {
    if (data.empty()) {
        utils::Logger::warn("Khali data compress karn lai bhejeya! Koi faida nahi.");
        return {};
    }

    z_stream stream{};
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;

    int ret = deflateInit2(&stream,
                           Z_DEFAULT_COMPRESSION,
                           Z_DEFLATED,
                           GZIP_WINDOW_BITS,   // 15 + 16 -> gzip container
                           8,                  // memLevel
                           Z_DEFAULT_STRATEGY);

    if (ret != Z_OK) {
        utils::Logger::error("Gzip initialization fail! Error code: " + std::to_string(ret));
        return {};
    }

    stream.avail_in = static_cast<uInt>(data.size());
    stream.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(data.data()));

    // Worst case for deflate is input + a small header/trailer overhead.
    std::vector<char> compressed(data.size() + 1024);
    stream.avail_out = static_cast<uInt>(compressed.size());
    stream.next_out = reinterpret_cast<Bytef*>(compressed.data());

    ret = deflate(&stream, Z_FINISH);
    if (ret != Z_STREAM_END) {
        utils::Logger::error("Compression vich dhamaal! Error: " + std::to_string(ret));
        deflateEnd(&stream);
        return {};
    }

    compressed.resize(stream.total_out);
    deflateEnd(&stream);

    const int savedPct = static_cast<int>(
        (1.0 - static_cast<double>(compressed.size()) / static_cast<double>(data.size())) * 100.0);
    utils::Logger::info("Gzip compress: " + std::to_string(data.size()) + " -> " +
                        std::to_string(compressed.size()) + " bytes (" +
                        std::to_string(savedPct) + "% chhota)");

    return compressed;
}

bool GzipCompressor::supportsGzip(const std::string& acceptEncoding) {
    if (acceptEncoding.empty()) {
        return false;
    }
    return utils::StringUtils::contains(utils::StringUtils::toLower(acceptEncoding), "gzip");
}

std::vector<std::string> GzipCompressor::parseAcceptEncoding(const std::string& acceptEncoding) {
    std::vector<std::string> encodings;

    if (acceptEncoding.empty()) {
        return encodings;
    }

    for (const auto& part : utils::StringUtils::split(acceptEncoding, ',')) {
        std::string encoding = utils::StringUtils::toLower(utils::StringUtils::trim(part));

        // Quality value (q=0.8) hatao
        const size_t semicolonPos = encoding.find(';');
        if (semicolonPos != std::string::npos) {
            encoding = utils::StringUtils::trim(encoding.substr(0, semicolonPos));
        }

        if (!encoding.empty()) {
            encodings.push_back(encoding);
        }
    }

    return encodings;
}

bool GzipCompressor::shouldCompress(const std::string& data, const std::string& contentType) {
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
