#pragma once

// FileHandler.hpp - File operations for HTTP file serving
// Files nu read/write karna - properly te safely!
// (Read/write files - properly and safely!)

#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include "utils/Logger.hpp"
#include "http/HttpConstants.hpp"

namespace handlers {

/**
 * FileHandler - Handle file operations for HTTP server
 * 
 * Purpose: Files nu safely read/write karo directory vichon
 *          (Safely read/write files from directory)
 * 
 * Security: Path traversal attacks se bachna (../ etc)
 * (Security: Protect against path traversal attacks)
 */
class FileHandler {
private:
    std::string baseDirectory_;  // Root directory for files
                                // Files iss folder vich hi allowed ne
                                // (Files only allowed in this folder)

public:
    /**
     * Constructor - Base directory set karo
     * (Constructor - Set base directory)
     */
    explicit FileHandler(const std::string& baseDir = ".") 
        : baseDirectory_(baseDir) {
        
        // Directory exist kardi hai ya nahi check karo
        // (Check if directory exists)
        if (!std::filesystem::exists(baseDirectory_)) {
            utils::Logger::warn("Base directory exist nahi kardi: " + baseDirectory_);
            // (Base directory doesn't exist)
            
            // Directory bana do agar nahi hai
            // (Create directory if it doesn't exist)
            try {
                std::filesystem::create_directories(baseDirectory_);
                utils::Logger::info("Directory bana ditti: " + baseDirectory_);
                // (Created directory)
            } catch (const std::exception& e) {
                utils::Logger::error("Directory banani fail: " + std::string(e.what()));
                // (Failed to create directory)
            }
        }
        
        utils::Logger::info("FileHandler tayar hai, base directory: " + baseDirectory_);
        // (FileHandler ready)
    }

    /**
     * File read karo
     * (Read file)
     * 
     * @param filename: Relative filename (e.g., "test.txt")
     * @return: File contents as vector<char>, empty if error
     */
    std::vector<char> readFile(const std::string& filename) {
        // Full path bana lo
        // (Build full path)
        std::filesystem::path fullPath = std::filesystem::path(baseDirectory_) / filename;
        
        // Security check - path traversal attack se bachao
        // (Security check - protect against path traversal)
        if (!isPathSafe(fullPath)) {
            utils::Logger::error("Oye! Path traversal attack ki try kar rahe ho? Denied: " + filename);
            // (Hey! Are you trying path traversal attack? Denied)
            return std::vector<char>();
        }

        // File exist kardi hai?
        // (Does file exist?)
        if (!std::filesystem::exists(fullPath)) {
            utils::Logger::warn("File nahi mili: " + fullPath.string());
            // (File not found)
            return std::vector<char>();
        }

        // File read karo binary mode vich
        // (Read file in binary mode)
        try {
            std::ifstream file(fullPath, std::ios::binary | std::ios::ate);
            
            if (!file.is_open()) {
                utils::Logger::error("File kholi nahi ja sakdi: " + fullPath.string());
                // (Can't open file)
                return std::vector<char>();
            }

            // File size pata karo
            // (Get file size)
            std::streamsize size = file.tellg();
            file.seekg(0, std::ios::beg);

            // Buffer vich read karo
            // (Read into buffer)
            std::vector<char> buffer(size);
            if (!file.read(buffer.data(), size)) {
                utils::Logger::error("File read karn vich problem: " + fullPath.string());
                // (Problem reading file)
                return std::vector<char>();
            }

            utils::Logger::info("File successfully read kitti: " + fullPath.string() + 
                              " (" + std::to_string(size) + " bytes)");
            // (File successfully read)

            return buffer;

        } catch (const std::exception& e) {
            utils::Logger::error("File read exception: " + std::string(e.what()));
            return std::vector<char>();
        }
    }

    /**
     * File write karo
     * (Write file)
     * 
     * @param filename: Relative filename
     * @param data: Data to write
     * @return: true if successful
     */
    bool writeFile(const std::string& filename, const std::string& data) {
        return writeFile(filename, std::vector<char>(data.begin(), data.end()));
    }

    /**
     * File write karo - binary data
     * (Write file - binary data)
     */
    bool writeFile(const std::string& filename, const std::vector<char>& data) {
        // Full path bana lo
        // (Build full path)
        std::filesystem::path fullPath = std::filesystem::path(baseDirectory_) / filename;
        
        // Security check
        if (!isPathSafe(fullPath)) {
            utils::Logger::error("Path traversal attack denied for write: " + filename);
            return false;
        }

        try {
            // Parent directory bana lo agar nahi hai
            // (Create parent directory if it doesn't exist)
            std::filesystem::create_directories(fullPath.parent_path());

            // File write karo
            // (Write file)
            std::ofstream file(fullPath, std::ios::binary);
            
            if (!file.is_open()) {
                utils::Logger::error("File write lai nahi khol sakde: " + fullPath.string());
                // (Can't open file for writing)
                return false;
            }

            file.write(data.data(), data.size());
            file.close();

            utils::Logger::info("File successfully write kitti: " + fullPath.string() + 
                              " (" + std::to_string(data.size()) + " bytes)");
            // (File successfully written)

            return true;

        } catch (const std::exception& e) {
            utils::Logger::error("File write exception: " + std::string(e.what()));
            return false;
        }
    }

    /**
     * File exist kardi hai ya nahi
     * (Check if file exists)
     */
    bool fileExists(const std::string& filename) {
        std::filesystem::path fullPath = std::filesystem::path(baseDirectory_) / filename;
        
        if (!isPathSafe(fullPath)) {
            return false;
        }

        return std::filesystem::exists(fullPath);
    }

    /**
     * File size return karo
     * (Return file size)
     */
    size_t getFileSize(const std::string& filename) {
        std::filesystem::path fullPath = std::filesystem::path(baseDirectory_) / filename;
        
        if (!isPathSafe(fullPath) || !std::filesystem::exists(fullPath)) {
            return 0;
        }

        try {
            return std::filesystem::file_size(fullPath);
        } catch (const std::exception& e) {
            utils::Logger::error("File size pata nahi chal sakda: " + std::string(e.what()));
            // (Can't determine file size)
            return 0;
        }
    }

    /**
     * File delete karo
     * (Delete file)
     */
    bool deleteFile(const std::string& filename) {
        std::filesystem::path fullPath = std::filesystem::path(baseDirectory_) / filename;
        
        if (!isPathSafe(fullPath)) {
            utils::Logger::error("Path unsafe hai, delete nahi kar sakde: " + filename);
            // (Path is unsafe, can't delete)
            return false;
        }

        try {
            if (std::filesystem::remove(fullPath)) {
                utils::Logger::info("File delete ho gayi: " + fullPath.string());
                // (File deleted)
                return true;
            } else {
                utils::Logger::warn("File delete nahi hoyi (exist nahi kardi?): " + fullPath.string());
                // (File not deleted - doesn't exist?)
                return false;
            }
        } catch (const std::exception& e) {
            utils::Logger::error("File delete exception: " + std::string(e.what()));
            return false;
        }
    }

    /**
     * Base directory return karo
     * (Get base directory)
     */
    const std::string& getBaseDirectory() const {
        return baseDirectory_;
    }

private:
    /**
     * Security check - path base directory vichon bahar toh nahi ja raha?
     * (Security check - is path going outside base directory?)
     * 
     * Prevents: "../../../etc/passwd" type attacks
     */
    bool isPathSafe(const std::filesystem::path& path) {
        try {
            // Absolute canonical paths compare karo
            // (Compare absolute canonical paths)
            auto canonicalBase = std::filesystem::canonical(baseDirectory_);
            auto canonicalPath = std::filesystem::weakly_canonical(path);
            
            // Check karo ki path base directory vich hai
            // (Check if path is within base directory)
            auto [baseEnd, pathEnd] = std::mismatch(
                canonicalBase.begin(), canonicalBase.end(),
                canonicalPath.begin(), canonicalPath.end()
            );
            
            // Agar base directory path da prefix hai toh safe hai
            // (If base directory is prefix of path, then it's safe)
            bool safe = (baseEnd == canonicalBase.end());
            
            if (!safe) {
                utils::Logger::warn("Unsafe path detected! Base: " + canonicalBase.string() + 
                                  ", Requested: " + canonicalPath.string());
            }
            
            return safe;
            
        } catch (const std::exception& e) {
            utils::Logger::error("Path safety check vich exception: " + std::string(e.what()));
            // (Exception in path safety check)
            return false;  // Agar doubt hai toh deny kar do
                          // (If in doubt, deny)
        }
    }
};

} // namespace handlers
