#include "parser.h"
#include <fstream>
#include <iostream>
#include <filesystem>

FLStudioData FLParser::getData(const std::string& filePath) {
    FLStudioData data;

    try {
        if (!std::filesystem::exists(filePath)) {
            std::cerr << "FL Studio state file not found: " << filePath << std::endl;
            return data;
        }

        std::ifstream file(filePath);
        if (!file.is_open()) {
            std::cerr << "Could not open FL Studio file state: " << filePath << std::endl;
            return data;
        }

        nlohmann::json jsonData;
        file >> jsonData;

        data.state = jsonData.value("state", "Idle");
        data.bpm = static_cast<int>(jsonData.value("bpm", 130.0));
        data.plugin = jsonData.value("plugin", "");
        data.projectName = jsonData.value("project_name", "");
        data.timestamp = jsonData.value("timestamp", 0);
    } catch (const std::exception& e) {
        std::cerr << "Error parsing FL Studio state file: " << e.what() << std::endl;
    }

    return data;
}

bool FLParser::isFileAvailable(const std::string& filePath) {
    return std::filesystem::exists(filePath);
}