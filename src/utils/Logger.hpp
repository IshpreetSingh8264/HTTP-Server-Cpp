#pragma once

// Logger.hpp - Thread-safe, level-filtered logging
// Saari application di logging ikko jagah - default level ERROR taan ki stdout/stderr pollute na ho
// (All application logging in one place - default level ERROR so stdout/stderr stays clean)

#include <mutex>
#include <string>

namespace utils {

/**
 * Logger - Thread-safe logging with level filtering
 *
 * Purpose: Program andar da kya ho reha hai, pata lage
 *          (See what is happening inside the program)
 *
 * Everything goes to stderr so that it can never corrupt a response body
 * written to stdout. The default threshold is ERROR; raise it to DEBUG for
 * development (see `Logger::setMinLevel`).
 */
class Logger {
public:
    // Kitna serious hai message - chhoti value = zyada serious
    // (How serious is the message - lower value = more serious)
    enum class Level {
        DEBUG,   // Developers lai debugging info
        INFO,    // Normal operation
        WARN,    // Dhyan de bhai!
        ERROR    // Kuch taan gadbad hai
    };

    /**
     * Set minimum level to log.
     * Messages below this level are discarded before formatting.
     */
    static void setMinLevel(Level level);

    static void debug(const std::string& message);
    static void info(const std::string& message);
    static void warn(const std::string& message);
    static void error(const std::string& message);

private:
    static std::mutex log_mutex_;  // Thread-safety lai
    static Level min_level_;        // Messages below this are skipped

    static std::string getCurrentTime();
    static const char* levelToString(Level level);
    static const char* getLevelColor(Level level);
    static void log(Level level, const std::string& message);
};

} // namespace utils
