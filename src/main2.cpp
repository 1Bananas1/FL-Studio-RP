#include "config.h"
#include "monitor.h"
#include <iostream>
#include <thread>
#include <chrono>

int main() {

    // Load the .env variables
    ConfigLoader config;
    if (config.loadEnvFile(".env")) {
        std::string discordId = config.getString("DISCORD_APPLICATION_ID", "NOT_FOUND");
        std::string stateFile = config.getString("STATE_FILE_PATH", "NOT_FOUND");
        int pollInterval = config.getInt("POLL_INTERVAL_MS", 999);
        bool debugMode = config.getBool("DEBUG_MODE", false);
        std::cout << "\n📋 Configuration Values:" << std::endl;
        std::cout << "  DISCORD_APPLICATION_ID: " << discordId << std::endl;
        std::cout << "  STATE_FILE_PATH: " << stateFile << std::endl;
        std::cout << "  POLL_INTERVAL_MS: " << pollInterval << std::endl;
        std::cout << "  DEBUG_MODE: " << (debugMode ? "true" : "false") << std::endl;
    }


    // Detect FL Studio
    ProcessMonitor monitor;
    if(monitor.searchForFLStudio()) {
        std::cout << "Found FL" << std::endl;
    }
    std::this_thread::sleep_for(std::chrono::seconds(10));
    
}