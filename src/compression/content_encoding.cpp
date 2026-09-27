#include "compression/content_encoding.hpp"

// content_encoding.cpp - Encoding token <-> enum mapping.

#include "utils/StringUtils.hpp"

namespace compression {

const char* toHeaderValue(ContentEncoding encoding) {
    switch (encoding) {
        case ContentEncoding::Gzip:    return "gzip";
        case ContentEncoding::Deflate: return "deflate";
        case ContentEncoding::Identity: break;
    }
    return "identity";
}

const char* toAcceptEncodingToken(ContentEncoding encoding) {
    return toHeaderValue(encoding);  // The two vocabularies are identical here.
}

ContentEncoding fromHeaderValue(const std::string& token) {
    const std::string lower = utils::StringUtils::toLower(utils::StringUtils::trim(token));

    if (lower == "gzip" || lower == "x-gzip") {
        return ContentEncoding::Gzip;
    }
    if (lower == "deflate") {
        return ContentEncoding::Deflate;
    }
    return ContentEncoding::Identity;
}

} // namespace compression
