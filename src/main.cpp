// main.cpp - HTTP Server Entry Point
// Yahan se sab shuru hunda hai!
// (Everything starts from here!)

#include <iostream>
#include <cstdlib>
#include <string>
#include <csignal>
#include "server/Server.hpp"
#include "utils/Logger.hpp"

// Global server instance - signal handling lai
// (Global server instance - for signal handling)
server::Server* globalServer = nullptr;

/**
 * Signal handler - Ctrl+C te graceful shutdown
 * (Signal handler - Graceful shutdown on Ctrl+C)
 */
void signalHandler(int signal) {
  if (signal == SIGINT || signal == SIGTERM) {
    utils::Logger::info("");
    utils::Logger::info("╔════════════════════════════════════════╗");
    utils::Logger::info("║  Shutdown signal mili! Band kar rahe  ║");
    utils::Logger::info("║  (Shutdown signal received! Stopping) ║");
    utils::Logger::info("╚════════════════════════════════════════╝");
    
    if (globalServer) {
      globalServer->stop();
    }
    exit(0);
  }
}

/**
 * Command line arguments parse karo
 * (Parse command line arguments)
 * 
 * Supported:
 * --directory <path>  : Base directory for file serving
 */
std::string parseDirectoryArg(int argc, char** argv) {
  std::string directory = ".";  // Default current directory
  
  // Arguments check karo
  // (Check arguments)
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    
    if (arg == "--directory" && i + 1 < argc) {
      // Next argument directory path hai
      // (Next argument is directory path)
      directory = argv[i + 1];
      utils::Logger::info("File directory argument mili: " + directory);
      // (File directory argument received)
      break;
    }
  }
  
  return directory;
}

/**
 * Main function - Server start karo!
 * (Main function - Start the server!)
 */
int main(int argc, char** argv) {
  // Output buffering off karo - logs turant dikhen
  // (Turn off output buffering - logs appear immediately)
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;
  
  utils::Logger::info("");
  utils::Logger::info("╔═══════════════════════════════════════════════════╗");
  utils::Logger::info("║                                                   ║");
  utils::Logger::info("║     🍛 HTTP Dhaba Server - Pinglish Edition 🍛    ║");
  utils::Logger::info("║        (HTTP Restaurant Server - Pinglish)        ║");
  utils::Logger::info("║                                                   ║");
  utils::Logger::info("║  Sat Sri Akaal! Server shuru kar rahe haan!      ║");
  utils::Logger::info("║  (Hello! Starting the server!)                   ║");
  utils::Logger::info("║                                                   ║");
  utils::Logger::info("╚═══════════════════════════════════════════════════╝");
  utils::Logger::info("");

  // Command line arguments parse karo
  // (Parse command line arguments)
  std::string fileDirectory = parseDirectoryArg(argc, argv);
  
  // Signal handlers setup - graceful shutdown lai
  // (Setup signal handlers - for graceful shutdown)
  std::signal(SIGINT, signalHandler);
  std::signal(SIGTERM, signalHandler);
  
  utils::Logger::info("Signal handlers set ho gaye (Ctrl+C lai)");
  // (Signal handlers set for Ctrl+C)

  try {
    // Server create karo te start karo!
    // (Create and start server!)
    server::Server httpServer(4221, fileDirectory);
    globalServer = &httpServer;
    
    utils::Logger::info("");
    utils::Logger::info("⚡ Server start ho raha hai... ⚡");
    utils::Logger::info("⚡ (Server starting...) ⚡");
    utils::Logger::info("");
    
    // Server chalao - yeh blocking call hai
    // (Run server - this is a blocking call)
    httpServer.start();
    
  } catch (const std::exception& e) {
    utils::Logger::error("");
    utils::Logger::error("╔═══════════════════════════════════════════╗");
    utils::Logger::error("║  Oye! Server crash ho gaya! Exception:   ║");
    utils::Logger::error("║  (Server crashed! Exception occurred!)   ║");
    utils::Logger::error("╚═══════════════════════════════════════════╝");
    utils::Logger::error(std::string(e.what()));
    return 1;
    
  } catch (...) {
    utils::Logger::error("");
    utils::Logger::error("╔═══════════════════════════════════════════╗");
    utils::Logger::error("║  Unknown exception! Kuch taan gadbad!    ║");
    utils::Logger::error("║  (Unknown exception! Something's wrong!) ║");
    utils::Logger::error("╚═══════════════════════════════════════════╝");
    return 1;
  }

  utils::Logger::info("");
  utils::Logger::info("╔═══════════════════════════════════════════╗");
  utils::Logger::info("║  Server band ho gaya. Phir milange!      ║");
  utils::Logger::info("║  (Server stopped. See you again!)        ║");
  utils::Logger::info("╚═══════════════════════════════════════════╝");
  utils::Logger::info("");

  return 0;
}
