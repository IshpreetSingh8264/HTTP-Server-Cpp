#pragma once

// content_encoding.hpp - The content codings this server can produce
// Server kinne encodings bana sakta hai - ek jagah.
// (Which content codings the server can produce - in one place.)

#include <string>

namespace compression {

/**
 * ContentEncoding - A content coding the server is able to emit
 *
 * `Identity` means "no transform": the body is sent as-is, which is what
 * RFC 9110 requires be the ultimate fallback.
 */
enum class ContentEncoding {
    Identity,
    Gzip,    // RFC 1952 - zlib deflate inside a gzip container
    Deflate  // RFC 1951 - a raw deflate stream
};

/// The exact token to put in a `Content-Encoding` response header.
const char* toHeaderValue(ContentEncoding encoding);

/// The `Accept-Encoding` token that requests this coding.
const char* toAcceptEncodingToken(ContentEncoding encoding);

/// Parse a `Content-Encoding` token. Unknown tokens become Identity.
ContentEncoding fromHeaderValue(const std::string& token);

} // namespace compression
