#include "config.h"
#include "monitor.h"
#include "parser.h"
#include "discord_rp.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <algorithm>


int main() {
    // Static Discord RPC instance - persists across function calls
    static DiscordRPC* discord = nullptr;
    static std::string discordId = "1396127471342194719";
    static long long sessionStartTime = 0;
    
    // Load configuration
    std::string stateFile = "Not_init";
    int pollInterval = 999;
    bool debugMode = false;

    ConfigLoader config;
    if (config.loadEnvFile(".env")) {
        stateFile = config.getString("STATE_FILE_PATH", "NOT_FOUND");
        pollInterval = config.getInt("POLL_INTERVAL_MS", 999);
        debugMode = config.getBool("DEBUG_MODE", false);
        config.setDebugMode(debugMode);
        if (debugMode) {
            std::cout << "\n📋 Configuration Values:" << std::endl;
            std::cout << "  DISCORD_APPLICATION_ID: " << discordId << std::endl;
            std::cout << "  STATE_FILE_PATH: " << stateFile << std::endl;
            std::cout << "  POLL_INTERVAL_MS: " << pollInterval << std::endl;
            std::cout << "  DEBUG_MODE: " << (debugMode ? "true" : "false") << std::endl;
        }
    } else {
        std::cout << "❌ Failed to load .env file!" << std::endl;
        std::cout << "Press Enter to exit..." << std::endl;
        std::cin.get();
        return 1;
    }

    // Initialize parser
    if (FLParser::isFileAvailable(stateFile)) {
        if (debugMode) {
            std::cout << "✅ State file found!" << std::endl;
        }
    } else {
        std::cout << "❌ State file not found!" << std::endl;
        std::cout << "Make sure FL Studio is running and the script is active." << std::endl;
        std::cout << "Press Enter to exit..." << std::endl;
        std::cin.get();
        return 1;
    }

    // Main monitoring loop
    ProcessMonitor monitor;
    monitor.setDebugMode(debugMode);
    
    while (true) {
        if (monitor.searchForFLStudio()) {
            if (debugMode) {
                std::cout << "Found FL Studio running..." << std::endl;
            }
            
            // If Discord client not initialized, create it
            if (discord == nullptr) {
                discord = new DiscordRPC(discordId);
                sessionStartTime = DiscordRPC::getCurrentTimestamp();
                if (debugMode) {
                    std::cout << "Initialized Discord RPC client" << std::endl;
                }
            }
            
            // If we are already connected
            if (discord->isConnected()) {
                // Get FL Studio data and update activity
                FLStudioData data = FLParser::getData(stateFile);
                
                
                DiscordActivity activity;
                activity.state = std::to_string(data.bpm) + " BPM";
                activity.details = data.state;
                if (!data.plugin.empty()) {
                    activity.details += " • " + data.plugin;
                }
                std::string lowerState = data.state;
                std::transform(lowerState.begin(), lowerState.end(), lowerState.begin(), ::tolower);
                activity.smallImage = lowerState;
                activity.largeImage = "fl_studio_logo";
                activity.largeText = "FL Studio";
                activity.startTime = sessionStartTime;
                
                
                if (discord->updateActivity(activity)) {
                    if (debugMode) {
                        std::cout << "✅ Rich Presence updated: " << data.state << " @ " << data.bpm << " BPM";
                        if (!data.plugin.empty()) {
                            std::cout << " (" << data.plugin << ")";
                        }
                        std::cout << std::endl;
                    }
                } else {
                    if (debugMode) {
                    std::cout << "❌ Failed to update Rich Presence" << std::endl;
                }
                }
                
            } else {
                // Try to connect
                if (discord->connect()) {
                    if (debugMode) {
                        std::cout << "✅ Connected to Discord!" << std::endl;
                    }
                    
                    // Set initial activity
                    DiscordActivity activity;
                    activity.state = "Starting up...";
                    activity.details = "FL Studio";
                    activity.largeImage = "fl_studio_logo";
                    activity.largeText = "FL Studio";
                    activity.smallImage = "idle";
                    activity.startTime = sessionStartTime;
                    
                    discord->updateActivity(activity);
                } else {
                    if (debugMode) {
                        std::cout << "❌ Failed to connect to Discord. Is Discord running?" << std::endl;
                    }
                }
            }
            
        } else {
            // FL Studio not running - disconnect if connected
            if (discord != nullptr && discord->isConnected()) {
                if (debugMode) {
                    std::cout << "FL Studio not running - clearing Discord presence" << std::endl;
                }
                discord->clearActivity();
                discord->disconnect();
                sessionStartTime = 0; // Reset timer for next session
            }
        }
        
        // Wait before next check
        std::this_thread::sleep_for(std::chrono::milliseconds(pollInterval));
    }
    
    // Cleanup
    if (discord != nullptr) {
        delete discord;
    }

    
}