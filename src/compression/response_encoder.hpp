#pragma once

// response_encoder.hpp - Compress a response body with zlib
// Response body nu compress karo - gzip te raw deflate dono.
// (Compress a response body - both gzip and raw deflate.)

#include <string>
#include <vector>

#include "compression/content_encoding.hpp"

namespace compression {

/**
 * ResponseEncoder - zlib-backed encoder for the codings we advertise
 *
 * Both codings are the same DEFLATE algorithm; they differ only in the
 * container:
 *
 *   gzip     windowBits = 15 + 16  -> zlib deflate wrapped in an RFC 1952
 *                                    gzip header/trailer
 *   deflate  windowBits = -15      -> a bare RFC 1951 deflate stream
 *
 * Note on `deflate`: RFC 9110 nominally defines the coding as the zlib format
 * (RFC 1950, a positive windowBits), but in practice every browser and curl
 * send and expect a raw RFC 1951 stream, which is what this produces. This is
 * the long-standing interop behaviour every other HTTP server implements.
 */
class ResponseEncoder {
public:
    /// windowBits values, named so the two containers are not confused again.
    static constexpr int GZIP_WINDOW_BITS = 15 + 16;  // gzip container
    static constexpr int DEFLATE_WINDOW_BITS = -15;    // raw deflate stream

    /// Compress into the gzip container. Empty vector on failure.
    static std::vector<char> gzip(const std::string& data);

    /// Compress into a raw deflate stream. Empty vector on failure.
    static std::vector<char> deflate(const std::string& data);

    /// Dispatch on the coding. Identity returns an empty vector by definition.
    static std::vector<char> compress(const std::string& data, ContentEncoding encoding);

    /// False for empty bodies and for content types that are already compressed.
    static bool shouldCompress(const std::string& data, const std::string& contentType);
};

} // namespace compression
