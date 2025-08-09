#include "config.h"
#include "monitor.h"
#include "parser.h"
#include "discord_rp.h"
#include "app_state.h"
#include "tray.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <algorithm>
#include <memory>


int main() {
    // Discord RPC instance - managed with smart pointer
    static std::unique_ptr<DiscordRPC> discord = nullptr;
    static std::string discordId = "1396127471342194719";
    static long long sessionStartTime = 0;
    static bool shouldExit = false;
    static bool userDisconnected = false;
    
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

    // Initialize system tray
    SystemTray tray;
    if (!tray.initialize("E:\\coding projects\\FL Studio RP\\public\\FLRP.ico", "FL Studio Rich Presence")) {
        std::cout << "❌ Failed to initialize system tray!" << std::endl;
        if (debugMode) {
            std::cout << "Continuing without system tray support..." << std::endl;
        }
    } else {
        // Set up tray callbacks
        tray.setRefreshConnectionCallback([&]() {
            if (discord && discord->isConnected()) {
                discord->disconnect();
            }
            userDisconnected = false; // Allow reconnection after refresh
            if (debugMode) {
                std::cout << "🔄 Refreshing Discord connection..." << std::endl;
            }
        });
        
        tray.setDisconnectCallback([&]() {
            if (discord && discord->isConnected()) {
                discord->clearActivity();
                discord->disconnect();
                userDisconnected = true; // Prevent automatic reconnection
                if (debugMode) {
                    std::cout << "🔌 Disconnected from Discord (user requested)" << std::endl;
                }
            }
        });
        
        tray.setExitCallback([&]() {
            shouldExit = true;
            if (debugMode) {
                std::cout << "🚪 Exit requested from system tray" << std::endl;
            }
        });
        
        tray.show();
        if (debugMode) {
            std::cout << "✅ System tray initialized" << std::endl;
        }
    }

    // Main monitoring loop
    ProcessMonitor monitor;
    monitor.setDebugMode(debugMode);
    
    while (!shouldExit) {
        if (monitor.searchForFLStudio()) {
            if (debugMode) {
                std::cout << "Found FL Studio running..." << std::endl;
            }
            
            // If Discord client not initialized, create it (unless user disconnected)
            if (discord == nullptr && !userDisconnected) {
                discord = std::make_unique<DiscordRPC>(discordId);
                sessionStartTime = DiscordRPC::getCurrentTimestamp();
                if (debugMode) {
                    std::cout << "Initialized Discord RPC client" << std::endl;
                }
            }
            
            // If we are already connected
            if (discord && discord->isConnected()) {
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
                
            } else if (discord && !userDisconnected) {
                // Try to connect (only if user hasn't manually disconnected)
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
        
        // Process tray messages
        tray.processMessages();
        
        // Wait before next check
        std::this_thread::sleep_for(std::chrono::milliseconds(pollInterval));
    }
    
    // Cleanup - smart pointer automatically handles deallocation
    discord.reset();

    
}