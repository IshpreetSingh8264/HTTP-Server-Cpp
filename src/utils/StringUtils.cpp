#include "utils/StringUtils.hpp"

// StringUtils.cpp - Pure string helpers shared by every layer above utils/.

#include <algorithm>
#include <cctype>
#include <sstream>

namespace utils {

std::vector<std::string> StringUtils::split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    size_t start = 0;

    while (true) {
        const size_t pos = str.find(delimiter, start);
        if (pos == std::string::npos) {
            tokens.push_back(str.substr(start));
            return tokens;
        }
        // Empty tokens are preserved - see StringUtils.hpp for why.
        tokens.push_back(str.substr(start, pos - start));
        start = pos + 1;
    }
}

std::vector<std::string> StringUtils::split(const std::string& str, const std::string& delimiter) {
    std::vector<std::string> tokens;
    if (delimiter.empty()) {
        tokens.push_back(str);
        return tokens;
    }

    size_t start = 0;
    while (true) {
        const size_t pos = str.find(delimiter, start);
        if (pos == std::string::npos) {
            tokens.push_back(str.substr(start));
            return tokens;
        }
        tokens.push_back(str.substr(start, pos - start));
        start = pos + delimiter.length();
    }
}

std::string StringUtils::trim(const std::string& str) {
    auto start = std::find_if(str.begin(), str.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    });

    auto end = std::find_if(str.rbegin(), str.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base();

    return (start < end) ? std::string(start, end) : "";
}

std::string StringUtils::toLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

bool StringUtils::equalsIgnoreCase(const std::string& str1, const std::string& str2) {
    if (str1.length() != str2.length()) {
        return false;
    }
    return std::equal(str1.begin(), str1.end(), str2.begin(),
                      [](char a, char b) { return std::tolower(a) == std::tolower(b); });
}

bool StringUtils::contains(const std::string& str, const std::string& substr) {
    return str.find(substr) != std::string::npos;
}

std::string StringUtils::substringBefore(const std::string& str, char delimiter) {
    const size_t pos = str.find(delimiter);
    if (pos == std::string::npos) {
        return str;
    }
    return str.substr(0, pos);
}

std::string StringUtils::substringAfter(const std::string& str, const std::string& delimiter) {
    const size_t pos = str.find(delimiter);
    if (pos == std::string::npos) {
        return "";
    }
    return str.substr(pos + delimiter.length());
}

bool StringUtils::startsWith(const std::string& str, const std::string& prefix) {
    if (str.length() < prefix.length()) {
        return false;
    }
    return str.compare(0, prefix.length(), prefix) == 0;
}

bool StringUtils::endsWith(const std::string& str, const std::string& suffix) {
    if (str.length() < suffix.length()) {
        return false;
    }
    return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
}

std::string StringUtils::urlDecode(const std::string& str) {
    std::string result;
    result.reserve(str.size());

    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '%' && i + 2 < str.size()) {
            int value = 0;
            std::istringstream iss(str.substr(i + 1, 2));
            if (iss >> std::hex >> value) {
                result += static_cast<char>(value);
                i += 2;
            } else {
                result += str[i];  // Invalid hex, keep the literal '%'
            }
        } else if (str[i] == '+') {
            result += ' ';  // Query-string convention
        } else {
            result += str[i];
        }
    }

    return result;
}

} // namespace utils
