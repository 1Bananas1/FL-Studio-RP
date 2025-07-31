#pragma once
#include <string>
#include "../lib/json.hpp"

struct FLStudioData {
    std::string state;
    int bpm;
    std::string plugin;
    std::string projectName; 
    int timestamp;

    FLStudioData() : state("Idle"), bpm(130), projectName(""), timestamp(0) {}
};

class FLParser {
    public:
        static FLStudioData getData(const std::string& filePath);
        static bool isFileAvailable(const std::string& filePath);
};