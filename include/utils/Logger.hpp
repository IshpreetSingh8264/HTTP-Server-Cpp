#pragma once

// Logger.hpp - Pinglish Style Logging System
// Sari application vich logging lai ekk hi jagah
// (Centralized logging for the entire application)

#include <iostream>
#include <string>
#include <mutex>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace utils {

/**
 * Logger class - Thread-safe logging with Pinglish humor
 * 
 * Purpose: Saanu program de andar ki ho reha hai, sab pata lagega
 *          (Helps us know what's happening inside the program)
 * 
 * Features:
 * - Thread-safe output (mutex protected)
 * - Timestamped messages
 * - Multiple log levels (DEBUG, INFO, WARN, ERROR)
 * - Pinglish + English bilingual messages for maximum fun!
 */
class Logger {
public:
    // Log levels - kitna serious hai message
    // (How serious is the message)
    enum class Level {
        DEBUG,   // Debugging info - developers lai
        INFO,    // General info - sab nu dikhega
        WARN,    // Warning - dhyan de bhai!
        ERROR    // Error - kuch taan gadbad hai
    };

private:
    static std::mutex log_mutex_;  // Thread-safety lai
                                   // (For thread-safety)
    static Level min_level_;       // Minimum level to log
    
    // Rang lagao terminal vich (ANSI colors)
    // (Add colors in terminal)
    static constexpr const char* COLOR_RESET = "\033[0m";
    static constexpr const char* COLOR_DEBUG = "\033[36m";  // Cyan
    static constexpr const char* COLOR_INFO = "\033[32m";   // Green
    static constexpr const char* COLOR_WARN = "\033[33m";   // Yellow
    static constexpr const char* COLOR_ERROR = "\033[31m";  // Red

    // Current time milega formatted
    // (Get current time formatted)
    static std::string getCurrentTime() {
        auto now = std::time(nullptr);
        auto tm = *std::localtime(&now);
        std::ostringstream oss;
        oss << std::put_time(&tm, "%H:%M:%S");
        return oss.str();
    }

    // Level ko string vich convert karo
    // (Convert level to string)
    static std::string levelToString(Level level) {
        switch (level) {
            case Level::DEBUG: return "DEBUG";
            case Level::INFO:  return "INFO";
            case Level::WARN:  return "WARN";
            case Level::ERROR: return "ERROR";
            default:           return "UNKNOWN";
        }
    }

    // Level ke liye rang milega
    // (Get color for level)
    static const char* getLevelColor(Level level) {
        switch (level) {
            case Level::DEBUG: return COLOR_DEBUG;
            case Level::INFO:  return COLOR_INFO;
            case Level::WARN:  return COLOR_WARN;
            case Level::ERROR: return COLOR_ERROR;
            default:           return COLOR_RESET;
        }
    }

    // Actual logging function - mutex protected
    static void log(Level level, const std::string& message) {
        if (level < min_level_) return; // Skip if below minimum level
        
        std::lock_guard<std::mutex> lock(log_mutex_);
        std::cerr << getLevelColor(level)
                  << "[" << getCurrentTime() << "] "
                  << "[" << levelToString(level) << "] "
                  << COLOR_RESET
                  << message << std::endl;
    }

public:
    // Debug level logging - developers lai debugging info
    // (For developers - debugging information)
    static void debug(const std::string& message) {
        log(Level::DEBUG, message);
    }

    // Info level - normal operation logs
    // Jado sab kuch sahi chal reha hai
    // (When everything is running smoothly)
    static void info(const std::string& message) {
        log(Level::INFO, message);
    }

    // Warning level - kuch taan gadbad lag rehi hai
    // (Something seems wrong but not critical)
    static void warn(const std::string& message) {
        log(Level::WARN, message);
    }

    // Error level - problem aa gayi hai, fix karo!
    // (A problem occurred, fix it!)
    static void error(const std::string& message) {
        log(Level::ERROR, message);
    }

    // Set minimum log level
    static void setMinLevel(Level level) {
        min_level_ = level;
    }
};

} // namespace utils
