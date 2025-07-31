#include <filesystem>
#include "../lib/json.hpp"

class ConfigLoader {
    private:
        nlohmann::json config;
        std::string findProjectRoot() const;
        std::string findEnvFile(const std::string& filename) const;

    public:
        ConfigLoader();

        bool loadEnvFile(const std::string& filename);

        std::string getString(const std::string& key, const std::string& defaultValue = "") const;
        int getInt(const std::string& key, int defaultValue = 0) const;
        bool getBool(const std::string& key, bool defaultValue = false) const;

        void printAll() const;
};
