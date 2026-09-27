// main.cpp - Thin entry point (Rule 1)
// Wire up options, install the signal handler, run the server. Nothing else.

#include <csignal>
#include <cstdlib>
#include <exception>
#include <string>
#include <unistd.h>

#include "server/Server.hpp"
#include "utils/Logger.hpp"

namespace {
constexpr int DEFAULT_PORT = 4221;  // CodeCrafters expects this port
}

static server::Server* g_runningServer = nullptr;

extern "C" void handleSignal(int signum) {
    if (g_runningServer != nullptr) {
        g_runningServer->stop();
    }
    // Async-signal-safe: no destructors, no stdio flush.
    _exit(signum == SIGINT ? 0 : 1);
}

namespace {

/// Parse argv. Only two options today: --directory <path> and --verbose.
struct Options {
    std::string directory = ".";
    bool verbose = false;
};

Options parseArgs(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--directory" && i + 1 < argc) {
            options.directory = argv[++i];
        } else if (arg == "--verbose" || arg == "-v") {
            options.verbose = true;
        }
    }
    return options;
}
} // namespace

int main(int argc, char** argv) {
    const Options options = parseArgs(argc, argv);

    // Quiet by default: a test harness reads the socket, not our stderr.
    utils::Logger::setMinLevel(options.verbose ? utils::Logger::Level::DEBUG
                                               : utils::Logger::Level::ERROR);

    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    try {
        server::Server httpServer(DEFAULT_PORT, options.directory);
        g_runningServer = &httpServer;
        httpServer.start();
    } catch (const std::exception& e) {
        utils::Logger::error("Server crash ho gaya: " + std::string(e.what()));
        return 1;
    } catch (...) {
        utils::Logger::error("Server crash ho gaya, unknown exception!");
        return 1;
    }

    return 0;
}
