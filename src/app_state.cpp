#include "app_state.h"
#include "discord_rp.h"
#include "monitor.h"
#include "parser.h"
#include <iostream>
#include <chrono>
#include <algorithm>

AppState::AppState() 
    : m_currentState(State::STOPPED)
    , m_shouldExit(false)
    , m_debugMode(false)
    , m_pollInterval(1000)
    , m_discordId("1396127471342194719")
    , m_sessionStartTime(0) {
}

AppState::~AppState() {
    stopMonitoring();
}

bool AppState::initialize(const std::string& stateFile, int pollInterval, bool debugMode) {
    m_stateFilePath = stateFile;
    m_pollInterval = pollInterval;
    m_debugMode.store(debugMode);
    
    // Initialize monitor
    m_monitor = std::make_unique<ProcessMonitor>();
    m_monitor->setDebugMode(debugMode);
    
    if (debugMode) {
        std::cout << "📋 AppState initialized:" << std::endl;
        std::cout << "  State file: " << m_stateFilePath << std::endl;
        std::cout << "  Poll interval: " << m_pollInterval << "ms" << std::endl;
        std::cout << "  Debug mode: " << (debugMode ? "enabled" : "disabled") << std::endl;
    }
    
    return true;
}

bool AppState::startMonitoring() {
    if (m_currentState.load() == State::MONITORING) {
        return true; // Already monitoring
    }
    
    // Check if FL Studio state file is available
    if (!FLParser::isFileAvailable(m_stateFilePath)) {
        if (m_debugMode.load()) {
            std::cout << "❌ FL Studio state file not found: " << m_stateFilePath << std::endl;
        }
        return false;
    }
    
    // Initialize Discord connection
    if (!initializeDiscord()) {
        if (m_debugMode.load()) {
            std::cout << "❌ Failed to initialize Discord RPC" << std::endl;
        }
        return false;
    }
    
    setState(State::MONITORING);
    m_sessionStartTime = DiscordRPC::getCurrentTimestamp();
    
    if (m_debugMode.load()) {
        std::cout << "✅ Started monitoring FL Studio" << std::endl;
    }
    
    return true;
}

bool AppState::stopMonitoring() {
    if (m_currentState.load() == State::STOPPED) {
        return true; // Already stopped
    }
    
    cleanupDiscord();
    setState(State::DISCONNECTED);
    
    if (m_debugMode.load()) {
        std::cout << "🔌 Stopped monitoring and disconnected from Discord" << std::endl;
    }
    
    return true;
}

bool AppState::refreshConnection() {
    bool wasMonitoring = (m_currentState.load() == State::MONITORING);
    
    // Clean up existing connection
    cleanupDiscord();
    
    if (wasMonitoring) {
        // Reinitialize if we were monitoring before
        if (initializeDiscord()) {
            setState(State::MONITORING);
            if (m_debugMode.load()) {
                std::cout << "🔄 Discord connection refreshed" << std::endl;
            }
            return true;
        } else {
            setState(State::DISCONNECTED);
            if (m_debugMode.load()) {
                std::cout << "❌ Failed to refresh Discord connection" << std::endl;
            }
            return false;
        }
    }
    
    setState(State::DISCONNECTED);
    return true;
}

bool AppState::update() {
    if (m_shouldExit.load()) {
        return false;
    }
    
    State currentState = m_currentState.load();
    
    if (currentState == State::MONITORING) {
        // Check if FL Studio is still running
        if (m_monitor->searchForFLStudio()) {
            // FL Studio is running, update Discord activity
            if (m_discord && m_discord->isConnected()) {
                updateDiscordActivity();
            } else {
                // Try to reconnect
                if (m_discord && m_discord->connect()) {
                    if (m_debugMode.load()) {
                        std::cout << "✅ Reconnected to Discord" << std::endl;
                    }
                } else if (m_debugMode.load()) {
                    std::cout << "❌ Failed to connect to Discord" << std::endl;
                }
            }
        } else {
            // FL Studio not running - clear activity but keep monitoring
            if (m_discord && m_discord->isConnected()) {
                m_discord->clearActivity();
                if (m_debugMode.load()) {
                    std::cout << "🔌 FL Studio not running - cleared Discord activity" << std::endl;
                }
            }
        }
    } else if (currentState == State::DISCONNECTED) {

    }
    
    return true;
}

void AppState::requestExit() {
    m_shouldExit.store(true);
    stopMonitoring();
    
    if (m_debugMode.load()) {
        std::cout << "🚪 Application exit requested" << std::endl;
    }
}

std::string AppState::getStatusString() const {
    State state = m_currentState.load();
    
    switch (state) {
        case State::STOPPED:
            return "Stopped";
            
        case State::MONITORING:
            if (m_discord && m_discord->isConnected()) {
                return "Connected - Monitoring FL Studio";
            } else {
                return "Monitoring FL Studio (Discord disconnected)";
            }
            
        case State::DISCONNECTED:
            return "Disconnected";
            
        default:
            return "Unknown";
    }
}

void AppState::setState(State newState) {
    m_currentState.store(newState);
}

bool AppState::initializeDiscord() {
    try {
        m_discord = std::make_unique<DiscordRPC>(m_discordId);
        
        if (m_discord->connect()) {
            // Set initial activity
            DiscordActivity activity;
            activity.state = "Starting up...";
            activity.details = "FL Studio";
            activity.largeImage = "fl_studio_logo";
            activity.largeText = "FL Studio";
            activity.smallImage = "idle";
            activity.startTime = m_sessionStartTime;
            
            m_discord->updateActivity(activity);
            
            if (m_debugMode.load()) {
                std::cout << "✅ Discord RPC initialized and connected" << std::endl;
            }
            
            return true;
        }
    } catch (const std::exception& e) {
        if (m_debugMode.load()) {
            std::cout << "❌ Discord RPC initialization failed: " << e.what() << std::endl;
        }
    }
    
    return false;
}

void AppState::cleanupDiscord() {
    if (m_discord) {
        if (m_discord->isConnected()) {
            m_discord->clearActivity();
            m_discord->disconnect();
        }
        m_discord.reset();
    }
}

bool AppState::updateDiscordActivity() {
    if (!m_discord || !m_discord->isConnected()) {
        return false;
    }
    
    try {
        FLStudioData data = FLParser::getData(m_stateFilePath);
        
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
        activity.startTime = m_sessionStartTime;
        
        bool success = m_discord->updateActivity(activity);
        
        if (m_debugMode.load() && success) {
            std::cout << "✅ Rich Presence updated: " << data.state << " @ " << data.bpm << " BPM";
            if (!data.plugin.empty()) {
                std::cout << " (" << data.plugin << ")";
            }
            std::cout << std::endl;
        }
        
        return success;
        
    } catch (const std::exception& e) {
        if (m_debugMode.load()) {
            std::cout << "❌ Failed to update Discord activity: " << e.what() << std::endl;
        }
        return false;
    }
}