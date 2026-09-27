#pragma once

// FileHandler.hpp - File operations for HTTP file serving
// Files nu read/write karna - properly te safely!
// (Read/write files - properly and safely!)

#include <filesystem>
#include <string>
#include <vector>

namespace handlers {

/**
 * FileHandler - Sandboxed file access for the /files/{name} routes
 *
 * Purpose: Files nu safely read/write karo directory vichon
 *          (Safely read/write files from one directory)
 *
 * Security: every path is canonicalised and rejected unless it sits inside the
 * base directory, which blocks "../../../etc/passwd" traversal. This is the
 * only component that touches the filesystem.
 */
class FileHandler {
private:
    std::string baseDirectory_;  // Files are confined to this folder

    /// True only if `path` canonicalises to something inside baseDirectory_.
    bool isPathSafe(const std::filesystem::path& path) const;

public:
    explicit FileHandler(const std::string& baseDir = ".");

    /// Read a whole file. Returns an empty vector on any failure.
    std::vector<char> readFile(const std::string& filename);

    bool writeFile(const std::string& filename, const std::string& data);
    bool writeFile(const std::string& filename, const std::vector<char>& data);

    /// Existence check, sandbox-aware. False for unsafe paths.
    bool fileExists(const std::string& filename);

    /// Size in bytes, or 0 when missing or unsafe.
    size_t getFileSize(const std::string& filename);

    /// Delete a file. False when missing, unsafe, or on I/O error.
    bool deleteFile(const std::string& filename);

    const std::string& getBaseDirectory() const { return baseDirectory_; }
};

} // namespace handlers
