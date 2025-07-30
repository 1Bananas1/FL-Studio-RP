#include "config.h"
#include <iostream>

int main() {
    std::cout << "🧪 Testing Config Loader" << std::endl;
    std::cout << "========================" << std::endl;
    
    // Create config loader
    ConfigLoader config;
    
    // Try to load .env file
    if (config.loadEnvFile(".env")) {
        std::cout << "✅ Config loaded successfully!" << std::endl;
        
        // Test getting values
        std::string discordId = config.getString("DISCORD_APPLICATION_ID", "NOT_FOUND");
        std::string stateFile = config.getString("STATE_FILE_PATH", "NOT_FOUND");
        int pollInterval = config.getInt("POLL_INTERVAL_MS", 999);
        bool debugMode = config.getBool("DEBUG_MODE", false);
        
        // Print results
        std::cout << "\n📋 Configuration Values:" << std::endl;
        std::cout << "  DISCORD_APPLICATION_ID: " << discordId << std::endl;
        std::cout << "  STATE_FILE_PATH: " << stateFile << std::endl;
        std::cout << "  POLL_INTERVAL_MS: " << pollInterval << std::endl;
        std::cout << "  DEBUG_MODE: " << (debugMode ? "true" : "false") << std::endl;
        
        // Print all config (for debugging)
        std::cout << "\n🔍 All Config Data:" << std::endl;
        config.printAll();
        
    } else {
        std::cout << "❌ Failed to load config!" << std::endl;
        std::cout << "Make sure you have a .env file with:" << std::endl;
        std::cout << "DISCORD_APPLICATION_ID=your_app_id" << std::endl;
        std::cout << "STATE_FILE_PATH=path_to_fl_state.json" << std::endl;
    }
    
    std::cout << "\nPress Enter to exit..." << std::endl;
    std::cin.get();
    
    return 0;
}