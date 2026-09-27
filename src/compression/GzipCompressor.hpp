#pragma once

// GzipCompressor.hpp - Gzip (RFC 1952) compression over zlib
// Data nu compress karke size ghatta lo!
// (Compress data to reduce size!)

#include <string>
#include <vector>

namespace compression {

/**
 * GzipCompressor - zlib-backed gzip codec
 *
 * Purpose: HTTP responses nu compress karke bandwidth bacha lo
 *          (Compress HTTP responses to save bandwidth)
 *
 * Flow: client sends "Accept-Encoding: gzip" -> server compresses the body ->
 * server sets "Content-Encoding: gzip" -> client inflates it.
 */
class GzipCompressor {
public:
    /// zlib windowBits for the gzip container: 15 (max window) + 16 (gzip wrapper).
    static constexpr int GZIP_WINDOW_BITS = 15 + 16;

    /**
     * Compress `data` into the gzip container.
     * @return Compressed bytes, or an empty vector when compression fails.
     */
    static std::vector<char> compress(const std::string& data);

    /// True when the Accept-Encoding header names gzip (case-insensitive).
    static bool supportsGzip(const std::string& acceptEncoding);

    /// Parse Accept-Encoding into bare encoding tokens, dropping ";q=..." params.
    /// "gzip, deflate;q=0.8" -> ["gzip", "deflate"]
    static std::vector<std::string> parseAcceptEncoding(const std::string& acceptEncoding);

    /// False for empty bodies and for content types that are already compressed.
    static bool shouldCompress(const std::string& data, const std::string& contentType);
};

} // namespace compression
