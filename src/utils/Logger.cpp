#include "utils/Logger.hpp"

// Logger.cpp - Level filtering, timestamping and mutex-protected output.

#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace utils {

std::mutex Logger::log_mutex_;

// Default threshold is ERROR. Anything looser floods stderr: at INFO a single
// request emits a dozen lines, which corrupts the output of any test harness
// reading the process. Raise it with --verbose for local development.
Logger::Level Logger::min_level_ = Logger::Level::ERROR;

namespace {
// Rang lagao terminal vich (ANSI colors)
constexpr const char* COLOR_RESET = "\033[0m";
constexpr const char* COLOR_DEBUG = "\033[36m";  // Cyan
constexpr const char* COLOR_INFO = "\033[32m";   // Green
constexpr const char* COLOR_WARN = "\033[33m";   // Yellow
constexpr const char* COLOR_ERROR = "\033[31m";  // Red
} // namespace

const char* Logger::levelToString(Level level) {
    switch (level) {
        case Level::DEBUG: return "DEBUG";
        case Level::INFO:  return "INFO";
        case Level::WARN:  return "WARN";
        case Level::ERROR: return "ERROR";
        default:           return "UNKNOWN";
    }
}

const char* Logger::getLevelColor(Level level) {
    switch (level) {
        case Level::DEBUG: return COLOR_DEBUG;
        case Level::INFO:  return COLOR_INFO;
        case Level::WARN:  return COLOR_WARN;
        case Level::ERROR: return COLOR_ERROR;
        default:           return COLOR_RESET;
    }
}

std::string Logger::getCurrentTime() {
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%H:%M:%S");
    return oss.str();
}

void Logger::setMinLevel(Level level) {
    std::lock_guard<std::mutex> lock(log_mutex_);
    min_level_ = level;
}

void Logger::log(Level level, const std::string& message) {
    // Cheap pre-check outside the lock; the authoritative check repeats under it.
    if (level < min_level_) return;

    std::lock_guard<std::mutex> lock(log_mutex_);
    if (level < min_level_) return;

    std::cerr << getLevelColor(level)
              << "[" << getCurrentTime() << "] "
              << "[" << levelToString(level) << "] "
              << COLOR_RESET
              << message << std::endl;
}

void Logger::debug(const std::string& message) { log(Level::DEBUG, message); }
void Logger::info(const std::string& message)  { log(Level::INFO, message); }
void Logger::warn(const std::string& message)  { log(Level::WARN, message); }
void Logger::error(const std::string& message) { log(Level::ERROR, message); }

} // namespace utils
