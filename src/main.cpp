#define DISCORDPP_IMPLEMENTATION
#include "discordpp.h"
#include <iostream>
#include <thread>
#include <fstream>
#include <atomic>
#include <string>
#include <functional>
#include <csignal>
#include "../lib/json.hpp"




// Create a flag to stop the application
std::atomic<bool> running = true;

// Signal handler to stop the application
void signalHandler(int signum) {
    running.store(false);
};

class FileParser {
  public:
    virtual ~FileParser() = default;
    virtual bool canParse(const std::string& filename) const = 0;
    virtual nlohmann::json parse(const std::string& filename) const = 0;
    virtual std::string getType() const = 0;
};

class JsonParser : public FileParser {
  public:
    bool canParse(const std::string& filename) const override {
      std::filesystem::path path(filename);
      std::string ext = path.extension().string();
      std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
      return ext == ".json";
    }

    nlohmann::json parse(const std::string& filename) const override {
      std::ifstream file(filename);
      if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filename);
      }

      nlohmann::json j;
      file >> j;
      return j;
    }

    std::string getType() const override {
      return "JSON";
    }

};

class EnvParser : public FileParser {
private:
    std::string findProjectRoot() const {
        std::filesystem::path currentPath = std::filesystem::current_path();
        
        std::vector<std::string> rootIndicators = {
            ".env", ".git", "CMakeLists.txt", "Makefile", "package.json", 
            ".gitignore", "README.md", "src", "include"
        };
        
        // Start from current directory and work up
        while (currentPath != currentPath.root_path()) {
            std::cout << "Checking directory: " << currentPath << std::endl;
            
            for (const auto& indicator : rootIndicators) {
                std::filesystem::path indicatorPath = currentPath / indicator;
                std::cout << "  Looking for: " << indicatorPath << std::endl;
                
                if (std::filesystem::exists(indicatorPath)) {
                    std::cout << "Found project root at: " << currentPath << std::endl;
                    return currentPath.string();
                }
            }
            currentPath = currentPath.parent_path();
        }
        
        // If no project root found, use current directory
        std::cout << "Project root not found, using current directory: " 
                  << std::filesystem::current_path() << std::endl;
        return std::filesystem::current_path().string();
    }
    
    std::string findEnvFile(const std::string& filename) const {
        // Try current directory first
        if (std::filesystem::exists(filename)) {
            return filename;
        }
        
        // Try project root
        std::string projectRoot = findProjectRoot();
        std::string envPath = projectRoot + "/" + filename;
        if (std::filesystem::exists(envPath)) {
            return envPath;
        }
        
        return ""; // Not found
    }
    
public:
    bool canParse(const std::string& filename) const override {
        std::filesystem::path path(filename);
        std::string ext = path.extension().string();
        std::string name = path.filename().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        
        return ext == ".env" || name == ".env" || name.find(".env") != std::string::npos;
    }
    
    nlohmann::json parse(const std::string& filename) const override {
        std::string actualPath = findEnvFile(filename);
        if (actualPath.empty()) {
            throw std::runtime_error("Environment file not found: " + filename);
        }
        
        std::ifstream file(actualPath);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open environment file: " + actualPath);
        }
        
        std::cout << "Loading environment variables from: " << actualPath << std::endl;
        
        nlohmann::json result;
        std::string line;
        
        while (std::getline(file, line)) {
            // Skip empty lines and comments
            if (line.empty() || line[0] == '#') {
                continue;
            }
            
            // Find the = delimiter
            size_t pos = line.find('=');
            if (pos != std::string::npos) {
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);
                
                // Trim whitespace
                key.erase(0, key.find_first_not_of(" \t"));
                key.erase(key.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);
                
                // Remove quotes if present
                if (value.size() >= 2 && 
                    ((value.front() == '"' && value.back() == '"') ||
                     (value.front() == '\'' && value.back() == '\''))) {
                    value = value.substr(1, value.size() - 2);
                }
                
                // Try to parse as number or boolean, otherwise keep as string
                if (value == "true" || value == "false") {
                    result[key] = (value == "true");
                } else {
                    try {
                        // For very large numbers (like Discord IDs), keep as string
                        if (value.length() > 15) {
                            result[key] = value;
                        } else if (value.find('.') == std::string::npos) {
                            // Try integer
                            result[key] = std::stoll(value);
                        } else {
                            // Try float
                            result[key] = std::stod(value);
                        }
                    } catch (const std::exception&) {
                        // Keep as string
                        result[key] = value;
                    }
                }
                
                // Debug output
                std::cout << "  Parsed: " << key << " = " << result[key] << " (type: " << result[key].type_name() << ")" << std::endl;
            }
        }
        
        return result;
    }
    
    std::string getType() const override {
        return "ENV";
    }
};

class UnifiedConfigLoader {
private:
    std::vector<std::unique_ptr<FileParser>> parsers;
    nlohmann::json config;
    
public:
    UnifiedConfigLoader() {
        // Register default parsers
        parsers.push_back(std::make_unique<JsonParser>());
        parsers.push_back(std::make_unique<EnvParser>());
    }
    
    void addParser(std::unique_ptr<FileParser> parser) {
        parsers.push_back(std::move(parser));
    }
    
    bool loadFile(const std::string& filename) {
        for (const auto& parser : parsers) {
            if (parser->canParse(filename)) {
                try {
                    nlohmann::json fileData = parser->parse(filename);
                    
                    // Merge with existing config
                    for (auto& [key, value] : fileData.items()) {
                        config[key] = value;
                    }
                    
                    std::cout << "Loaded " << fileData.size() << " entries from " 
                              << parser->getType() << " file: " << filename << std::endl;
                    return true;
                } catch (const std::exception& e) {
                    std::cerr << "Error parsing " << parser->getType() 
                              << " file: " << e.what() << std::endl;
                    return false;
                }
            }
        }
        
        std::cerr << "No suitable parser found for file: " << filename << std::endl;
        return false;
    }
    
    // Type-safe getters
    template<typename T>
    T get(const std::string& key, const T& defaultValue = T{}) const {
        if (config.contains(key)) {
            try {
                return config[key].get<T>();
            } catch (const std::exception&) {
                std::cerr << "Warning: Could not convert " << key 
                          << " to requested type, using default" << std::endl;
            }
        }
        return defaultValue;
    }
    
    // Convenience methods
    std::string getString(const std::string& key, const std::string& defaultValue = "") const {
        return get<std::string>(key, defaultValue);
    }
    
    int getInt(const std::string& key, int defaultValue = 0) const {
        return get<int>(key, defaultValue);
    }
    
    bool getBool(const std::string& key, bool defaultValue = false) const {
        return get<bool>(key, defaultValue);
    }
    
    double getDouble(const std::string& key, double defaultValue = 0.0) const {
        return get<double>(key, defaultValue);
    }
    
    const nlohmann::json& getJson() const {
        return config;
    }
    
    void printAll() const {
        std::cout << "Configuration:" << std::endl;
        std::cout << config.dump(2) << std::endl;
    }
};

class UnifiedFileMonitor {
private:
    std::string filename;
    std::filesystem::file_time_type lastModified;
    nlohmann::json currentData;
    UnifiedConfigLoader loader;
    std::function<void(const nlohmann::json&)> callback;
    bool running;
    std::thread monitorThread;
    int pollIntervalMs;
    
    void monitorLoop() {
        while (running) {
            try {
                if (std::filesystem::exists(filename)) {
                    auto currentModified = std::filesystem::last_write_time(filename);
                    
                    if (currentModified != lastModified) {
                        std::cout << "File changed, reloading..." << std::endl;
                        
                        UnifiedConfigLoader newLoader;
                        if (newLoader.loadFile(filename)) {
                            currentData = newLoader.getJson();
                            loader = std::move(newLoader);
                            lastModified = currentModified;
                            
                            if (callback) {
                                callback(currentData);
                            }
                        }
                    }
                }
            } catch (const std::exception& e) {
                std::cerr << "Error monitoring file: " << e.what() << std::endl;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(pollIntervalMs));
        }
    }
    
public:
    UnifiedFileMonitor(const std::string& file, int pollInterval = 1000) 
        : filename(file), running(false), pollIntervalMs(pollInterval) {
        
        // Initial load
        if (loader.loadFile(filename)) {
            currentData = loader.getJson();
            if (std::filesystem::exists(filename)) {
                lastModified = std::filesystem::last_write_time(filename);
            }
            std::cout << "Initial file loaded successfully" << std::endl;
        }
    }
    
    ~UnifiedFileMonitor() {
        stop();
    }
    
    void setCallback(std::function<void(const nlohmann::json&)> cb) {
        callback = cb;
    }
    
    void start() {
        if (!running) {
            running = true;
            monitorThread = std::thread(&UnifiedFileMonitor::monitorLoop, this);
            std::cout << "File monitoring started for: " << filename << std::endl;
        }
    }
    
    void stop() {
        if (running) {
            running = false;
            if (monitorThread.joinable()) {
                monitorThread.join();
            }
            std::cout << "File monitoring stopped" << std::endl;
        }
    }
    
    const UnifiedConfigLoader& getLoader() const {
        return loader;
    }
    
    const nlohmann::json& getCurrentData() const {
        return currentData;
    }
};

int main() {
  // Look and load config
    UnifiedConfigLoader config;
    if (!config.loadFile("../.env")) {  // Try parent directory first
      if (!config.loadFile(".env")) {  // Fallback to current directory
        if (!config.loadFile("../../.env")) {  // Try two levels up
            std::cerr << "Failed to load .env file. Exiting." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(2));
            return 1;
          }
      }
    }

    std::string discordAppId = config.getString("DISCORD_APPLICATION_ID");
    if (discordAppId.empty()) {
        std::cerr << "DISCORD_APPLICATION_ID not found in .env file" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
        return 1;
    }
    const uint64_t APPLICATION_ID = std::stoull(discordAppId);

    std::string stateFilePath = config.getString("STATE_FILE_PATH");
    if (stateFilePath.empty()) {
        std::cerr << "STATE_FILE_PATH not found in .env file" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
        return 1;
    }

    if (!std::filesystem::exists(stateFilePath)) {
        std::cerr << "FL Studio state file not found: " << stateFilePath << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
        return 1;
    }

    int pollInterval = config.getInt("POLL_INTERVAL_MS", 500);
    bool debugMode = config.getBool("DEBUG_MODE", false);

    // DEBUG STARTER CODE

    std::cout << "Discord Application ID: " << discordAppId << std::endl;
    std::cout << "Monitoring FL Studio state: " << stateFilePath << std::endl;
    std::cout << "Poll interval: " << pollInterval << "ms" << std::endl;

    if (debugMode) {
        std::cout << "Debug mode enabled" << std::endl;
    }

    // STart of the discord initializing code

    std::signal(SIGINT, signalHandler);
    std::cout << "🚀 Initializing Discord SDK...\n";

    // Create our Discord Client
    auto client = std::make_shared<discordpp::Client>();

    // Set up logging callback
    client->AddLogCallback([](auto message, auto severity) {
      std::cout << "[" << EnumToString(severity) << "] " << message << std::endl;
    }, discordpp::LoggingSeverity::Info);

    // Set up status callback to monitor client connection
    client->SetStatusChangedCallback([client](discordpp::Client::Status status, discordpp::Client::Error error, int32_t errorDetail) {
      std::cout << "🔄 Status changed: " << discordpp::Client::StatusToString(status) << std::endl;

      if (status == discordpp::Client::Status::Ready) {
        std::cout << "✅ Client is ready! You can now call SDK functions.\n";

        // Configure rich presence details
        discordpp::Activity activity;
        activity.SetType(discordpp::ActivityTypes::Playing);
        activity.SetState("NaN");
        activity.SetDetails("NaN");

        // Update rich presence
        client->UpdateRichPresence(activity, [](discordpp::ClientResult result) {

          if(result.Successful()) {
            std::cout << "🎮 Rich Presence updated successfully!\n";
          } else {
            std::cerr << "❌ Rich Presence update failed";
          }
        });

      } else if (error != discordpp::Client::Error::None) {
        std::cerr << "❌ Connection Error: " << discordpp::Client::ErrorToString(error) << " - Details: " << errorDetail << std::endl;
      }
    });

    
    // might remove later, just a pain in the ass tbh
    // Generate OAuth2 code verifier for authentication
    auto codeVerifier = client->CreateAuthorizationCodeVerifier();

    // Set up authentication arguments
    discordpp::AuthorizationArgs args{};
    args.SetClientId(APPLICATION_ID);
    args.SetScopes(discordpp::Client::GetDefaultPresenceScopes());
    args.SetCodeChallenge(codeVerifier.Challenge());

    // Begin authentication process
    client->Authorize(args, [client, codeVerifier, APPLICATION_ID](auto result, auto code, auto redirectUri) {
      if (!result.Successful()) {
        std::cerr << "❌ Authentication Error: " << result.Error() << std::endl;
        return;
      } else {
        std::cout << "✅ Authorization successful! Getting access token...\n";

        // Exchange auth code for access token
        client->GetToken(APPLICATION_ID, code, codeVerifier.Verifier(), redirectUri,
          [client, APPLICATION_ID](discordpp::ClientResult result,
          std::string accessToken,
          std::string refreshToken,
          discordpp::AuthorizationTokenType tokenType,
          int32_t expiresIn,
          std::string scope) {
            std::cout << "🔓 Access token received! Establishing connection...\n";
            // Next Step: Update the token and connect
            client->UpdateToken(discordpp::AuthorizationTokenType::Bearer,  accessToken, [client](discordpp::ClientResult result) {
              if(result.Successful()) {
                std::cout << "🔑 Token updated, connecting to Discord...\n";
                client->Connect();
              }
            });
        });
      }
    });
    // monitoring
    UnifiedFileMonitor stateMonitor(stateFilePath, pollInterval);
    stateMonitor.setCallback([client](const nlohmann::json& flState) {
    static nlohmann::json lastState; // Keep track of last state to avoid unnecessary updates
    
    try {
        // Extract FL Studio information
        std::string state = flState.value("state", "Unknown");
        std::string projectName = flState.value("project_name", "");
        std::string plugin = flState.value("plugin", "");
        double bpm = flState.value("bpm", 120.0);
        int64_t timestamp = flState.value("timestamp", static_cast<int64_t>(time(nullptr)));
        
        std::string stateKey = state + "|" + std::to_string((int)bpm) + "|" + plugin + "|" + projectName;
        std::string lastKey = "";
        if (!lastState.empty()) {
            lastKey = lastState.value("state", "") + "|" + 
                     std::to_string(lastState.value("bpm", 0)) + "|" + 
                     lastState.value("plugin", "") + "|" + 
                     lastState.value("project_name", "");
        }
        
        // Only update if state actually changed
        if (stateKey == lastKey) {
            return; // No change, skip update
        }
        
        std::cout << "=== FL Studio State Changed ===" << std::endl;
        std::cout << "State: " << state << std::endl;
        std::cout << "Project: " << (projectName.empty() ? "Unnamed Project" : projectName) << std::endl;
        std::cout << "BPM: " << (int)bpm << std::endl;
        std::cout << "Plugin: " << (plugin.empty() ? "None" : plugin) << std::endl;
        
        // Create Discord Rich Presence
        discordpp::Activity activity;
        activity.SetType(discordpp::ActivityTypes::Playing);
        
        // Set state (bottom line) - always show BPM like your Python version
        std::string activityState = std::to_string((int)bpm) + " BPM";
        
        // Set details (top line) based on FL Studio state
        std::string activityDetails;
        
        if (state == "Recording") {
            activityDetails = "Recording";
        } else if (state == "Listening") {
            activityDetails = "Listening to Track";
        } else if (state == "Composing") {
            activityDetails = "Composing";
        } else if (state == "Playing") {
            activityDetails = "Playing";
        } else {
            activityDetails = "Idle";
        }
        
        // Add plugin info if available (like your Python version)
        if (!plugin.empty()) {
            activityDetails += " • " + plugin;
        }
        
        // Add project name if available
        if (!projectName.empty()) {
            activityDetails += " - " + projectName;
        }
        
        activity.SetState(activityState);
        activity.SetDetails(activityDetails);
        
        // Set timestamps
        // activity.SetStartTime(timestamp);
        
        // Update rich presence
        client->UpdateRichPresence(activity, [activityDetails, bpm, plugin](discordpp::ClientResult result) {
            if(result.Successful()) {
                std::cout << "🔄 Updated Discord: " << activityDetails << " @ " << (int)bpm << " BPM";
                if (!plugin.empty()) {
                    std::cout << " (" << plugin << ")";
                }
                std::cout << std::endl;
            } else {
                std::cerr << "❌ Rich Presence update failed: " << result.Error() << std::endl;
            }
        });
        
        // Update last state
        lastState = flState;
        
    } catch (const std::exception& e) {
        std::cerr << "Error processing FL Studio state: " << e.what() << std::endl;
    }
    
    std::cout << "================================" << std::endl;
});

    stateMonitor.start();
    std::cout << "📁 Started monitoring FL Studio state file...\n";

    // Keep application running to allow SDK to receive events and callbacks
    while (running) {
      discordpp::RunCallbacks();
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return 0;
}
