#pragma once

// encoding_negotiator.hpp - Accept-Encoding -> one Content-Encoding
// Client ki list suno, apni list se match karo, ek chuno.
// (Read the client's list, match it against ours, pick one.)

#include <string>
#include <vector>

#include "compression/content_encoding.hpp"

namespace compression {

/// One entry of a parsed `Accept-Encoding` header.
struct EncodingPreference {
    ContentEncoding encoding = ContentEncoding::Identity;
    bool wildcard = false;   // the "*" token
    double quality = 1.0;    // q= weight, 0.0 means "not acceptable"
    int order = 0;           // position in the client's list; lower is better
};

/**
 * EncodingNegotiator - Picks the content coding for a response
 *
 * Algorithm, in priority order:
 *   1. Highest q wins. An explicit `coding;q=0` makes that coding unusable.
 *   2. On a q tie, the coding the client listed first wins, which is the
 *      behaviour every browser and proxy relies on.
 *   3. A bare `*` accepts any coding we can produce.
 *   4. `identity` is the fallback: it is always acceptable, so an absent or
 *      unusable Accept-Encoding yields an uncompressed response.
 *
 * Server preference only ever breaks a tie between two codings the client
 * listed in the same order as our internal order, which cannot happen because
 * step 2 already resolves by client order.
 */
class EncodingNegotiator {
public:
    /// Split, lower-case, and read the `;q=` weights out of the header.
    static std::vector<EncodingPreference> parse(const std::string& acceptEncoding);

    /// Choose the coding to use. Returns Identity when nothing matches.
    static ContentEncoding negotiate(const std::string& acceptEncoding);
};

} // namespace compression
