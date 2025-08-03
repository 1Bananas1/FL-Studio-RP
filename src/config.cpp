#include "config.h"
#include <algorithm>
#include <iostream>
#include <vector>
#include <fstream>

ConfigLoader::ConfigLoader() {
    
}

std::string ConfigLoader::findProjectRoot() const {
    std::filesystem::path currentPath = std::filesystem::current_path();

    std::vector<std::string> rootIndicators = {
        ".env", ".git", "CMakeLists.txt", "Makefile", ".gitignore", "src"
    };

    while (currentPath != currentPath.root_path()) {
        for (const auto& indicator : rootIndicators) {
            std::filesystem::path indicatorPath = currentPath / indicator;
            if (std::filesystem::exists(indicatorPath)) {
                return currentPath.string();
            }
        }
        currentPath = currentPath.parent_path();
    }
    return std::filesystem::current_path().string();
 }

std::string ConfigLoader::findEnvFile(const std::string& filename) const {

    if (std::filesystem::exists(filename)) {
        return filename;
    }

    std::string projectRoot = findProjectRoot();
    std::string envPath = projectRoot + "/" + filename;
    if (std::filesystem::exists(envPath)) {
        return envPath;
    }

    return "";
}


bool ConfigLoader::loadEnvFile(const std::string& filename) {
    std::string actualPath = findEnvFile(filename);
    if (actualPath.empty()) {
        std::cerr << "Environment file not found: " << filename << std::endl;
        return false;
    }

    std::ifstream file(actualPath);
    if (!file.is_open()) {
        std::cerr << "Cannot open environment file: " << actualPath << std::endl;
        return false;
    }

    if (debugMode) {
        std::cout << "Loading environment variables from: " << actualPath << std::endl;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }

        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);

            // get rid of the whitespace
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);

            if (value.size() >= 2 &&
                ((value.front() == '""' && value.back() == '""') ||
                (value.front() == '\'' && value.back() == '\''))) {
                value = value.substr(1, value.size() - 2);
            }

            config[key] = value;
        }
    }

    return true;
}

std::string ConfigLoader::getString(const std::string& key, const std::string& defaultValue) const {
    if ( config.contains(key)) {
        return config[key].get<std::string>();
    }
    return defaultValue;
}

int ConfigLoader::getInt(const std::string& key, int defaultValue) const {
    if (config.contains(key)) {
        try {
            return std::stoi(config[key].get<std::string>());
        } catch (const std::exception&) {
            return defaultValue;
        }
    }
    return defaultValue;
}

bool ConfigLoader::getBool(const std::string& key, bool defaultValue) const {
    if (config.contains(key)) {
        std::string value = config[key].get<std::string>();
        std::transform(value.begin(), value.end(), value.begin(), ::tolower);
        return value == "true" || value == "1" || value == "yes";
    }
    return defaultValue;
}

void ConfigLoader::setDebugMode(bool debug) {
    debugMode = debug;
}

void ConfigLoader::printAll() const {
    if (debugMode) {
        std::cout << "Configuration:" << std::endl;
        std::cout << config.dump(2) << std::endl;
    }
}