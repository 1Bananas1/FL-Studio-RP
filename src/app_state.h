#ifndef APP_STATE_H
#define APP_STATE_H

#include <string>
#include <atomic>
#include <memory>

// Forward declarations
class DiscordRPC;
class ProcessMonitor;

/**
 * @brief Application state manager
 * 
 * Manages the overall state of the FL Studio Rich Presence application,
 * including Discord connection status and FL Studio monitoring state.
 */
class AppState {
public:
    enum class State {
        STOPPED,        // Application stopped, no monitoring
        MONITORING,     // Monitoring FL Studio, Discord connected
        DISCONNECTED    // Monitoring stopped, Discord disconnected
    };

private:
    std::atomic<State> m_currentState;
    std::atomic<bool> m_shouldExit;
    std::atomic<bool> m_debugMode;
    
    // Configuration
    std::string m_stateFilePath;
    int m_pollInterval;
    std::string m_discordId;
    
    // Runtime objects
    std::unique_ptr<DiscordRPC> m_discord;
    std::unique_ptr<ProcessMonitor> m_monitor;
    long long m_sessionStartTime;

public:
    /**
     * @brief Constructor
     */
    AppState();
    
    /**
     * @brief Destructor
     */
    ~AppState();
    
    /**
     * @brief Initialize application state with configuration
     * @param stateFile Path to FL Studio state file
     * @param pollInterval Polling interval in milliseconds
     * @param debugMode Whether debug mode is enabled
     * @return true if successful, false otherwise
     */
    bool initialize(const std::string& stateFile, int pollInterval, bool debugMode);
    
    /**
     * @brief Start monitoring FL Studio and connect to Discord
     * @return true if successful, false otherwise
     */
    bool startMonitoring();
    
    /**
     * @brief Stop monitoring and disconnect from Discord
     * @return true if successful, false otherwise
     */
    bool stopMonitoring();
    
    /**
     * @brief Refresh Discord connection (disconnect and reconnect)
     * @return true if successful, false otherwise
     */
    bool refreshConnection();
    
    /**
     * @brief Perform one monitoring cycle
     * Should be called regularly from main loop
     * @return true to continue, false if should exit
     */
    bool update();
    
    /**
     * @brief Request application exit
     */
    void requestExit();
    
    /**
     * @brief Check if application should exit
     * @return true if should exit, false otherwise
     */
    bool shouldExit() const { return m_shouldExit.load(); }
    
    /**
     * @brief Get current application state
     * @return Current state
     */
    State getCurrentState() const { return m_currentState.load(); }
    
    /**
     * @brief Get debug mode status
     * @return true if debug mode enabled, false otherwise
     */
    bool isDebugMode() const { return m_debugMode.load(); }
    
    /**
     * @brief Get current status string for display
     * @return Status string describing current state
     */
    std::string getStatusString() const;

private:
    /**
     * @brief Set internal state
     * @param newState New state to set
     */
    void setState(State newState);
    
    /**
     * @brief Initialize Discord RPC connection
     * @return true if successful, false otherwise
     */
    bool initializeDiscord();
    
    /**
     * @brief Clean up Discord RPC connection
     */
    void cleanupDiscord();
    
    /**
     * @brief Update Discord Rich Presence with current FL Studio data
     * @return true if successful, false otherwise
     */
    bool updateDiscordActivity();
};

#endif // APP_STATE_H