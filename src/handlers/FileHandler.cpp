#include "handlers/FileHandler.hpp"

// FileHandler.cpp - Sandboxed filesystem access.

#include <algorithm>
#include <exception>
#include <fstream>

#include "utils/Logger.hpp"

namespace handlers {

FileHandler::FileHandler(const std::string& baseDir) : baseDirectory_(baseDir) {
    if (!std::filesystem::exists(baseDirectory_)) {
        utils::Logger::warn("Base directory exist nahi kardi: " + baseDirectory_);
        try {
            std::filesystem::create_directories(baseDirectory_);
            utils::Logger::info("Directory bana ditti: " + baseDirectory_);
        } catch (const std::exception& e) {
            utils::Logger::error("Directory banani fail: " + std::string(e.what()));
        }
    }
    utils::Logger::info("FileHandler tayar, base directory: " + baseDirectory_);
}

std::vector<char> FileHandler::readFile(const std::string& filename) {
    const std::filesystem::path fullPath = std::filesystem::path(baseDirectory_) / filename;

    if (!isPathSafe(fullPath)) {
        utils::Logger::error("Oye! Path traversal attack ki try kar rahe ho? Denied: " + filename);
        return {};
    }

    if (!std::filesystem::exists(fullPath)) {
        utils::Logger::warn("File nahi mili: " + fullPath.string());
        return {};
    }

    try {
        std::ifstream file(fullPath, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            utils::Logger::error("File kholi nahi ja sakdi: " + fullPath.string());
            return {};
        }

        const std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<char> buffer(static_cast<size_t>(size));
        if (size > 0 && !file.read(buffer.data(), size)) {
            utils::Logger::error("File read karn vich problem: " + fullPath.string());
            return {};
        }

        utils::Logger::info("File read kitti: " + fullPath.string() +
                            " (" + std::to_string(size) + " bytes)");
        return buffer;

    } catch (const std::exception& e) {
        utils::Logger::error("File read exception: " + std::string(e.what()));
        return {};
    }
}

bool FileHandler::writeFile(const std::string& filename, const std::string& data) {
    return writeFile(filename, std::vector<char>(data.begin(), data.end()));
}

bool FileHandler::writeFile(const std::string& filename, const std::vector<char>& data) {
    const std::filesystem::path fullPath = std::filesystem::path(baseDirectory_) / filename;

    if (!isPathSafe(fullPath)) {
        utils::Logger::error("Path traversal attack denied for write: " + filename);
        return false;
    }

    try {
        if (fullPath.has_parent_path()) {
            std::filesystem::create_directories(fullPath.parent_path());
        }

        std::ofstream file(fullPath, std::ios::binary);
        if (!file.is_open()) {
            utils::Logger::error("File write lai nahi khol sakde: " + fullPath.string());
            return false;
        }

        if (!data.empty()) {
            file.write(data.data(), static_cast<std::streamsize>(data.size()));
        }
        file.close();

        utils::Logger::info("File write kitti: " + fullPath.string() +
                            " (" + std::to_string(data.size()) + " bytes)");
        return true;

    } catch (const std::exception& e) {
        utils::Logger::error("File write exception: " + std::string(e.what()));
        return false;
    }
}

bool FileHandler::fileExists(const std::string& filename) {
    const std::filesystem::path fullPath = std::filesystem::path(baseDirectory_) / filename;
    if (!isPathSafe(fullPath)) {
        return false;
    }
    return std::filesystem::exists(fullPath);
}

size_t FileHandler::getFileSize(const std::string& filename) {
    const std::filesystem::path fullPath = std::filesystem::path(baseDirectory_) / filename;

    if (!isPathSafe(fullPath) || !std::filesystem::exists(fullPath)) {
        return 0;
    }

    try {
        return std::filesystem::file_size(fullPath);
    } catch (const std::exception& e) {
        utils::Logger::error("File size pata nahi chal sakda: " + std::string(e.what()));
        return 0;
    }
}

bool FileHandler::deleteFile(const std::string& filename) {
    const std::filesystem::path fullPath = std::filesystem::path(baseDirectory_) / filename;

    if (!isPathSafe(fullPath)) {
        utils::Logger::error("Path unsafe hai, delete nahi kar sakde: " + filename);
        return false;
    }

    try {
        if (std::filesystem::remove(fullPath)) {
            utils::Logger::info("File delete ho gayi: " + fullPath.string());
            return true;
        }
        utils::Logger::warn("File delete nahi hoyi (exist nahi kardi?): " + fullPath.string());
        return false;
    } catch (const std::exception& e) {
        utils::Logger::error("File delete exception: " + std::string(e.what()));
        return false;
    }
}

bool FileHandler::isPathSafe(const std::filesystem::path& path) const {
    try {
        const auto canonicalBase = std::filesystem::canonical(baseDirectory_);
        const auto canonicalPath = std::filesystem::weakly_canonical(path);

        // Safe only if base is a full path-component prefix of the target.
        // A plain string prefix would let "/data/root2" pass a "/data/root" base.
        const auto [baseEnd, pathEnd] = std::mismatch(
            canonicalBase.begin(), canonicalBase.end(),
            canonicalPath.begin(), canonicalPath.end());

        const bool safe = (baseEnd == canonicalBase.end());
        if (!safe) {
            utils::Logger::warn("Unsafe path! Base: " + canonicalBase.string() +
                                ", Requested: " + canonicalPath.string());
        }
        return safe;

    } catch (const std::exception& e) {
        utils::Logger::error("Path safety check vich exception: " + std::string(e.what()));
        return false;  // Agar doubt hai toh deny kar do
    }
}

} // namespace handlers
