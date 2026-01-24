#pragma once

// GzipCompressor.hpp - Data compression using gzip (zlib)
// Data nu compress karke size ghatta lo!
// (Compress data to reduce size!)

#include <vector>
#include <string>
#include <zlib.h>
#include "utils/Logger.hpp"
#include "utils/StringUtils.hpp"

namespace compression {

/**
 * GzipCompressor - Gzip compression using zlib library
 * 
 * Purpose: HTTP responses nu compress karke bandwidth bacha lo
 *          (Compress HTTP responses to save bandwidth)
 * 
 * How it works:
 * - Client sends "Accept-Encoding: gzip"
 * - Server compresses response body
 * - Server sends "Content-Encoding: gzip"
 * - Client decompresses the data
 * 
 * Benefits: Bandwidth save, faster transfer!
 */
class GzipCompressor {
public:
    /**
     * Data nu gzip format vich compress karo
     * (Compress data in gzip format)
     * 
     * @param data: Original uncompressed data
     * @return: Compressed data as vector<char>
     * 
     * Uses zlib with gzip wrapper (windowBits = 15 + 16)
     */
    static std::vector<char> compress(const std::string& data) {
        if (data.empty()) {
            utils::Logger::warn("Khali data compress karn lai bhejeya! Koi faida nahi.");
            // (Empty data sent for compression! No point.)
            return std::vector<char>();
        }

        // zlib stream setup
        z_stream stream;
        stream.zalloc = Z_NULL;     // Memory allocation function
        stream.zfree = Z_NULL;      // Memory free function
        stream.opaque = Z_NULL;     // Private data pointer

        // deflateInit2 - gzip format lai
        // (deflateInit2 - for gzip format)
        // windowBits = 15 + 16 means gzip format (15 is default, +16 adds gzip wrapper)
        int ret = deflateInit2(&stream, 
                              Z_DEFAULT_COMPRESSION,  // Compression level (6)
                              Z_DEFLATED,             // Compression method
                              15 + 16,                // windowBits (gzip format)
                              8,                      // memLevel (default)
                              Z_DEFAULT_STRATEGY);    // Strategy

        if (ret != Z_OK) {
            utils::Logger::error("Oye! Gzip initialization fail ho gayi! Error code: " + std::to_string(ret));
            // (Hey! Gzip initialization failed!)
            return std::vector<char>();
        }

        // Input data set karo
        // (Set input data)
        stream.avail_in = data.size();
        stream.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(data.data()));

        // Output buffer - compressed data iththe ayega
        // (Output buffer - compressed data will come here)
        std::vector<char> compressed;
        compressed.resize(data.size() + 1024);  // Extra space for headers/overhead

        stream.avail_out = compressed.size();
        stream.next_out = reinterpret_cast<Bytef*>(compressed.data());

        // Compression karo!
        // (Do the compression!)
        ret = deflate(&stream, Z_FINISH);
        
        if (ret != Z_STREAM_END) {
            utils::Logger::error("Compression vich dhamaal! Error: " + std::to_string(ret));
            // (Compression went haywire!)
            deflateEnd(&stream);
            return std::vector<char>();
        }

        // Compressed size check karo
        // (Check compressed size)
        size_t compressedSize = stream.total_out;
        compressed.resize(compressedSize);

        // Cleanup
        deflateEnd(&stream);

        // Compression ratio dekho - kitna space bachya!
        // (Check compression ratio - how much space saved!)
        float ratio = (1.0f - static_cast<float>(compressedSize) / data.size()) * 100.0f;
        utils::Logger::info("Gzip ne kamaal kar ditta! Original: " + std::to_string(data.size()) + 
                          " bytes, Compressed: " + std::to_string(compressedSize) + 
                          " bytes (" + std::to_string(static_cast<int>(ratio)) + "% bachya!)");
        // (Gzip did wonders! Original: X bytes, Compressed: Y bytes (Z% saved!))

        return compressed;
    }

    /**
     * Check karo client gzip support karda hai ya nahi
     * (Check if client supports gzip)
     * 
     * @param acceptEncoding: Value of Accept-Encoding header
     * @return: true if gzip is supported
     * 
     * Example: "Accept-Encoding: gzip, deflate" -> true
     */
    static bool supportsGzip(const std::string& acceptEncoding) {
        if (acceptEncoding.empty()) {
            return false;
        }

        // Accept-Encoding nu lowercase vich convert karke check karo
        // (Convert Accept-Encoding to lowercase and check)
        std::string lower = utils::StringUtils::toLower(acceptEncoding);
        
        // "gzip" substring milega ya nahi
        // (Check if "gzip" substring is present)
        bool supported = utils::StringUtils::contains(lower, "gzip");
        
        if (supported) {
            utils::Logger::debug("Client gzip support karda hai! Compression ON!");
            // (Client supports gzip! Compression ON!)
        } else {
            utils::Logger::debug("Client gzip support nahi karda. No compression.");
            // (Client doesn't support gzip. No compression.)
        }
        
        return supported;
    }

    /**
     * Check karo ki data compress karn da faida hai ya nahi
     * (Check if it's worth compressing the data)
     * 
     * Small files (< 1KB) compress karn da koi faida nahi
     * (No point compressing small files)
     */
    static bool shouldCompress(const std::string& data, const std::string& contentType) {
        // Note: CodeCrafters tests chote data te bhi compression expect karde ne
        // (Note: CodeCrafters tests expect compression even on small data)
        // Production vich, normally 1KB se chota data compress nahi karna chahida
        // (In production, normally shouldn't compress data smaller than 1KB)
        
        // Empty data compress nahi karni
        // (Don't compress empty data)
        if (data.empty()) {
            return false;
        }

        // Binary data (images, videos) already compressed hunde ne
        // (Binary data is already compressed)
        std::string lower = utils::StringUtils::toLower(contentType);
        if (utils::StringUtils::contains(lower, "image/") ||
            utils::StringUtils::contains(lower, "video/") ||
            utils::StringUtils::contains(lower, "audio/") ||
            utils::StringUtils::contains(lower, "application/zip") ||
            utils::StringUtils::contains(lower, "application/gzip")) {
            
            utils::Logger::debug("Content type already compressed hai: " + contentType + ", skip.");
            // (Content type is already compressed, skip.)
            return false;
        }

        // Text data compress karke faida hai
        // (Text data benefits from compression)
        return true;
    }

    /**
     * Parse Accept-Encoding header te saare supported encodings return karo
     * (Parse Accept-Encoding header and return all supported encodings)
     * 
     * Example: "gzip, deflate, br" -> ["gzip", "deflate", "br"]
     */
    static std::vector<std::string> parseAcceptEncoding(const std::string& acceptEncoding) {
        std::vector<std::string> encodings;
        
        if (acceptEncoding.empty()) {
            return encodings;
        }

        // Comma se split karo
        // (Split by comma)
        auto parts = utils::StringUtils::split(acceptEncoding, ',');
        
        for (const auto& part : parts) {
            // Trim te lowercase
            std::string encoding = utils::StringUtils::toLower(utils::StringUtils::trim(part));
            
            // Quality value (q=0.8) hatao agar hai
            // (Remove quality value if present)
            size_t semicolonPos = encoding.find(';');
            if (semicolonPos != std::string::npos) {
                encoding = encoding.substr(0, semicolonPos);
                encoding = utils::StringUtils::trim(encoding);
            }
            
            if (!encoding.empty()) {
                encodings.push_back(encoding);
            }
        }
        
        return encodings;
    }
};

} // namespace compression
