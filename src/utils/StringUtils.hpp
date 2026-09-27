#pragma once

// StringUtils.hpp - String parsing te manipulation utilities
// Strings nu todna, jodna, saaf karna - sab kuch!
// (Breaking, cleaning and comparing strings - everything!)

#include <string>
#include <vector>

namespace utils {

/**
 * StringUtils - Stateless string helpers
 *
 * Purpose: HTTP parsing lai zaroori string operations
 *          (Essential string operations for HTTP parsing)
 *
 * Every method is a pure static function: no state, no I/O, no domain
 * knowledge. Implementation lives in StringUtils.cpp.
 */
class StringUtils {
public:
    /**
     * Split by a single-character delimiter, preserving empty tokens.
     *
     * "a,,b" -> ["a", "", "b"]
     *
     * Empty tokens are kept so that positional parses (the request line, for
     * example) cannot silently shift: "GET  /x HTTP/1.1" yields 4 parts and is
     * rejected, rather than quietly becoming a valid 3-part line.
     */
    static std::vector<std::string> split(const std::string& str, char delimiter);

    /**
     * Split by a multi-character delimiter, preserving empty tokens.
     *
     * "key: value\r\nkey2: value2" split by "\r\n" -> ["key: value", "key2: value2"]
     */
    static std::vector<std::string> split(const std::string& str, const std::string& delimiter);

    /// Trim ASCII whitespace from both ends. "  hi  " -> "hi"
    static std::string trim(const std::string& str);

    /// Lowercase a copy of the string.
    static std::string toLower(const std::string& str);

    /// Case-insensitive equality (HTTP header names are case-insensitive).
    static bool equalsIgnoreCase(const std::string& str1, const std::string& str2);

    static bool contains(const std::string& str, const std::string& substr);

    /// Part before the first occurrence of `delimiter`, or the whole string.
    static std::string substringBefore(const std::string& str, char delimiter);

    /// Part after the first occurrence of `delimiter`, or "" if absent.
    static std::string substringAfter(const std::string& str, const std::string& delimiter);

    static bool startsWith(const std::string& str, const std::string& prefix);
    static bool endsWith(const std::string& str, const std::string& suffix);

    /// Percent-decoding. "hello%20world" -> "hello world"; '+' also becomes a space.
    static std::string urlDecode(const std::string& str);
};

} // namespace utils
