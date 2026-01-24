#include "utils/Logger.hpp"

// Logger.cpp - Static member initialization
// Logger de static members nu initialize karo
// (Initialize Logger's static members)

namespace utils {

// Mutex for thread-safe logging
// Sare threads peacefully log kar sakde
// (All threads can log peacefully)
std::mutex Logger::log_mutex_;

// Default minimum level is INFO
// Normally INFO level tak hi dikhao, debugging nahi
// (Normally show only INFO level, not debugging)
Logger::Level Logger::min_level_ = Logger::Level::INFO;

} // namespace utils
