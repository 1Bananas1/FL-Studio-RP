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
#include <filesystem>
#include <windows.h>


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
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
        
        // Allocate console only in debug mode
        if (debugMode) {
            AllocConsole();
            freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
            freopen_s((FILE**)stderr, "CONOUT$", "w", stderr);
            freopen_s((FILE**)stdin, "CONIN$", "r", stdin);
            std::ios::sync_with_stdio(true);
            std::wcout.clear();
            std::cout.clear();
            std::wcerr.clear();
            std::cerr.clear();
            std::wcin.clear();
            std::cin.clear();
        }
        
        if (debugMode) {
            std::cout << "\n📋 Configuration Values:" << std::endl;
            std::cout << "  DISCORD_APPLICATION_ID: " << discordId << std::endl;
            std::cout << "  STATE_FILE_PATH: " << stateFile << std::endl;
            std::cout << "  POLL_INTERVAL_MS: " << pollInterval << std::endl;
            std::cout << "  DEBUG_MODE: " << (debugMode ? "true" : "false") << std::endl;
        }
    } else {
        if (debugMode) {
            std::cout << "❌ Failed to load .env file!" << std::endl;
        }
        MessageBox(NULL, "Failed to load .env file! Please reinstall the application.", "FL Studio Rich Presence", MB_ICONERROR);
        return 1;
    }

    // Initialize parser
    if (FLParser::isFileAvailable(stateFile)) {
        if (debugMode) {
            std::cout << "✅ State file found!" << std::endl;
        }
    } else {
        if (debugMode) {
            std::cout << "❌ State file not found!" << std::endl;
            std::cout << "Make sure FL Studio is running and the script is active." << std::endl;
        }
        // Don't exit immediately - let it run and wait for FL Studio to start
        if (debugMode) {
            std::cout << "Waiting for FL Studio to start..." << std::endl;
        }
    }

    // Get executable directory and build icon path
    char exePath[MAX_PATH];
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    std::filesystem::path executableDir = std::filesystem::path(exePath).parent_path();
    std::string iconPath = (executableDir / "FLRP.ico").string();
    
    // Initialize system tray
    SystemTray tray;
    if (!tray.initialize(iconPath, "FL Studio Rich Presence")) {
        if (debugMode) {
            std::cout << "❌ Failed to initialize system tray!" << std::endl;
            std::cout << "Continuing without system tray support..." << std::endl;
        }
        // In non-debug mode, show error and exit since tray is essential for GUI-less app
        if (!debugMode) {
            MessageBox(NULL, "Failed to initialize system tray! The application will exit.", "FL Studio Rich Presence", MB_ICONERROR);
            return 1;
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
    
    return 0;
}