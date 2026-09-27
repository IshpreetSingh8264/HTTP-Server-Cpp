#include "compression/encoding_negotiator.hpp"

// encoding_negotiator.cpp - Accept-Encoding parsing and coding selection.

#include <algorithm>
#include <cmath>
#include <utility>
#include <sstream>

#include "utils/Logger.hpp"
#include "utils/StringUtils.hpp"

namespace compression {

namespace {

/// Codings this server can actually produce, most preferred first.
constexpr ContentEncoding kOffered[] = {ContentEncoding::Gzip, ContentEncoding::Deflate};

/// Read `q=0.8` out of a parameter list. Returns 1.0 when absent, 0.0 if
/// unparseable (an unreadable weight must not become a preference).
double parseQuality(const std::string& parameters) {
    for (const auto& parameter : utils::StringUtils::split(parameters, ';')) {
        const std::string trimmed = utils::StringUtils::trim(parameter);
        if (!utils::StringUtils::startsWith(trimmed, "q=")) {
            continue;
        }
        try {
            size_t consumed = 0;
            const double q = std::stod(trimmed.substr(2), &consumed);
            if (consumed == 0 || q < 0.0 || q > 1.0) {
                return 0.0;
            }
            return q;
        } catch (const std::exception&) {
            return 0.0;
        }
    }
    return 1.0;
}

} // namespace

std::vector<EncodingPreference> EncodingNegotiator::parse(const std::string& acceptEncoding) {
    std::vector<EncodingPreference> preferences;

    int order = 0;
    for (const auto& element : utils::StringUtils::split(acceptEncoding, ',')) {
        const std::string trimmed = utils::StringUtils::trim(element);
        if (trimmed.empty()) {
            continue;
        }

        EncodingPreference preference;
        preference.order = order++;

        // Split the token from its parameters at the first ';'.
        const size_t semicolon = trimmed.find(';');
        std::string token = (semicolon == std::string::npos)
                                ? trimmed
                                : trimmed.substr(0, semicolon);
        token = utils::StringUtils::toLower(utils::StringUtils::trim(token));

        if (semicolon != std::string::npos) {
            preference.quality = parseQuality(trimmed.substr(semicolon + 1));
        }

        if (token == "*") {
            preference.wildcard = true;
            preference.encoding = ContentEncoding::Identity;
        } else if (token == "identity") {
            preference.encoding = ContentEncoding::Identity;
        } else {
            preference.encoding = fromHeaderValue(token);
            if (preference.encoding == ContentEncoding::Identity && token != "identity") {
                // A coding we do not implement (br, zstd, ...). Keep it in the
                // list so it still occupies its position, but it can never be
                // chosen because it is not in kOffered.
                continue;
            }
        }

        preferences.push_back(preference);
    }

    return preferences;
}

namespace {

/// How well one coding matched the client's list.
struct Offer {
    bool present = false;
    double quality = 0.0;
    int order = 0;
};

/// The best offer for one coding: highest q, then earliest position. A `*`
/// token matches every coding, so it is scored alongside explicit tokens
/// rather than as a separate fallback.
Offer scoreFor(ContentEncoding candidate, const std::vector<EncodingPreference>& preferences) {
    Offer best;

    for (const auto& preference : preferences) {
        if (!preference.wildcard && preference.encoding != candidate) {
            continue;
        }
        if (!best.present || preference.quality > best.quality ||
            (preference.quality == best.quality && preference.order < best.order)) {
            best = Offer{true, preference.quality, preference.order};
        }
    }

    return best;
}

} // namespace

ContentEncoding EncodingNegotiator::negotiate(const std::string& acceptEncoding) {
    // An empty header means the client accepts anything, but a server that
    // compresses unasked is a server that breaks naive clients. Stay identity.
    if (utils::StringUtils::trim(acceptEncoding).empty()) {
        return ContentEncoding::Identity;
    }

    const std::vector<EncodingPreference> preferences = parse(acceptEncoding);

    ContentEncoding best = ContentEncoding::Identity;
    Offer bestOffer;

    for (const ContentEncoding candidate : kOffered) {
        const Offer offer = scoreFor(candidate, preferences);

        // Absent, or explicitly refused with q=0.
        if (!offer.present || offer.quality <= 0.0) {
            continue;
        }

        if (!bestOffer.present || offer.quality > bestOffer.quality ||
            (offer.quality == bestOffer.quality && offer.order < bestOffer.order)) {
            best = candidate;
            bestOffer = offer;
        }
    }

    utils::Logger::debug(std::string("Accept-Encoding '") + acceptEncoding + "' -> " +
                         toHeaderValue(best));
    return best;
}

} // namespace compression
