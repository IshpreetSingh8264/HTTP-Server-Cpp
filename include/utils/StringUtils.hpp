#pragma once

// StringUtils.hpp - String parsing te manipulation utilities
// Strings nu todna, jodna, saaf karna - sab kuch!
// (Breaking, joining, cleaning strings - everything!)

#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <string_view>
#include <sstream>

namespace utils {

/**
 * StringUtils - String manipulation helpers
 * 
 * Purpose: HTTP parsing lai zaroori string operations
 *          (Essential string operations for HTTP parsing)
 * 
 * Features:
 * - Split strings by delimiter
 * - Trim whitespace
 * - Case-insensitive comparison
 * - URL decoding
 * - Header parsing helpers
 */
class StringUtils {
public:
    /**
     * String nu split karo delimiter se
     * (Split string by delimiter)
     * 
     * Example: "GET /path HTTP/1.1" -> ["GET", "/path", "HTTP/1.1"]
     */
    static std::vector<std::string> split(const std::string& str, char delimiter) {
        std::vector<std::string> tokens;
        std::string token;
        
        // Har character te jao te delimiter dhundho
        // (Go through each character and find delimiter)
        for (char ch : str) {
            if (ch == delimiter) {
                if (!token.empty()) {
                    tokens.push_back(token);
                    token.clear();
                }
            } else {
                token += ch;
            }
        }
        
        // Last token vi add karo ji
        // (Add the last token too)
        if (!token.empty()) {
            tokens.push_back(token);
        }
        
        return tokens;
    }

    /**
     * String nu split karo multi-character delimiter se
     * (Split string by multi-character delimiter)
     * 
     * Example: "key: value\r\nkey2: value2" split by "\r\n"
     */
    static std::vector<std::string> split(const std::string& str, const std::string& delimiter) {
        std::vector<std::string> tokens;
        size_t start = 0;
        size_t end = str.find(delimiter);
        
        // Delimiter milega toh string tod do
        // (If delimiter found, break the string)
        while (end != std::string::npos) {
            tokens.push_back(str.substr(start, end - start));
            start = end + delimiter.length();
            end = str.find(delimiter, start);
        }
        
        // Last piece vi add karo
        // (Add last piece too)
        tokens.push_back(str.substr(start));
        
        return tokens;
    }

    /**
     * Whitespace saaf karo string ke aage-piche se
     * (Remove whitespace from start and end of string)
     * 
     * Example: "  hello  " -> "hello"
     */
    static std::string trim(const std::string& str) {
        // Pehle te last non-whitespace character dhundho
        // (Find first and last non-whitespace character)
        auto start = std::find_if(str.begin(), str.end(), [](unsigned char ch) {
            return !std::isspace(ch);
        });
        
        auto end = std::find_if(str.rbegin(), str.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
        }).base();
        
        // Agar sab whitespace hai toh khali string return karo
        // (If all whitespace, return empty string)
        return (start < end) ? std::string(start, end) : "";
    }

    /**
     * String ko lowercase vich convert karo
     * (Convert string to lowercase)
     * 
     * Useful for case-insensitive header comparison
     */
    static std::string toLower(const std::string& str) {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(),
                      [](unsigned char c) { return std::tolower(c); });
        return result;
    }

    /**
     * Case-insensitive comparison - uppercase/lowercase vich farak nahi
     * (Case-insensitive comparison - no difference in upper/lowercase)
     * 
     * HTTP headers are case-insensitive: "Content-Type" == "content-type"
     */
    static bool equalsIgnoreCase(const std::string& str1, const std::string& str2) {
        // Length check - agar alag hai toh different hain
        // (Length check - if different, strings are different)
        if (str1.length() != str2.length()) {
            return false;
        }
        
        // Har character compare karo lowercase vich
        // (Compare each character in lowercase)
        return std::equal(str1.begin(), str1.end(), str2.begin(),
                         [](char a, char b) {
                             return std::tolower(a) == std::tolower(b);
                         });
    }

    /**
     * String vich substring hai ki nahi check karo
     * (Check if string contains substring)
     */
    static bool contains(const std::string& str, const std::string& substr) {
        return str.find(substr) != std::string::npos;
    }

    /**
     * String vich koi bhi delimiter se pehle ka part return karo
     * (Return part of string before any delimiter)
     * 
     * Example: "GET /path HTTP/1.1" -> "GET" (before space)
     */
    static std::string substringBefore(const std::string& str, char delimiter) {
        size_t pos = str.find(delimiter);
        if (pos == std::string::npos) {
            return str; // Delimiter nahi mileya toh puri string
                       // (Delimiter not found, return whole string)
        }
        return str.substr(0, pos);
    }

    /**
     * String vich delimiter ke baad ka part return karo
     * (Return part of string after delimiter)
     * 
     * Example: "Content-Type: text/plain" -> "text/plain" (after ": ")
     */
    static std::string substringAfter(const std::string& str, const std::string& delimiter) {
        size_t pos = str.find(delimiter);
        if (pos == std::string::npos) {
            return ""; // Delimiter nahi mileya toh khali string
                      // (Delimiter not found, return empty)
        }
        return str.substr(pos + delimiter.length());
    }

    /**
     * String prefix check - string iss prefix se shuru hundi hai?
     * (Check if string starts with prefix)
     */
    static bool startsWith(const std::string& str, const std::string& prefix) {
        if (str.length() < prefix.length()) {
            return false;
        }
        return str.substr(0, prefix.length()) == prefix;
    }

    /**
     * String suffix check - string iss suffix te khatam hundi hai?
     * (Check if string ends with suffix)
     */
    static bool endsWith(const std::string& str, const std::string& suffix) {
        if (str.length() < suffix.length()) {
            return false;
        }
        return str.substr(str.length() - suffix.length()) == suffix;
    }

    /**
     * URL decode karo - %20 nu space vich convert karo
     * (URL decode - convert %20 to space, etc.)
     * 
     * Example: "hello%20world" -> "hello world"
     */
    static std::string urlDecode(const std::string& str) {
        std::string result;
        
        for (size_t i = 0; i < str.length(); ++i) {
            if (str[i] == '%' && i + 2 < str.length()) {
                // Hex code nu decode karo
                // (Decode hex code)
                int value;
                std::istringstream iss(str.substr(i + 1, 2));
                if (iss >> std::hex >> value) {
                    result += static_cast<char>(value);
                    i += 2;
                } else {
                    result += str[i]; // Invalid hex, just add %
                }
            } else if (str[i] == '+') {
                // + ko space vich convert karo (query strings vich)
                // (Convert + to space in query strings)
                result += ' ';
            } else {
                result += str[i];
            }
        }
        
        return result;
    }
};

} // namespace utils
