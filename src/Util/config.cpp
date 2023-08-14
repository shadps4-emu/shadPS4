#include "config.h"

#include <fstream>
#include <string>
#include <toml11/toml.hpp>

namespace Config {

bool isNeo = false;

bool isNeoMode() { return isNeo; }
void load(const std::filesystem::path& path) {
    // If the configuration file does not exist, create it and return
    std::error_code error;
    if (!std::filesystem::exists(path, error)) {
        save(path);
        return;
    }

    toml::value data;

    try {
        data = toml::parse(path);
    } catch (std::exception& ex) {
        printf("Got exception trying to load config file. Exception: %s\n", ex.what());
        return;
    }

    if (data.contains("General")) {
        auto generalResult = toml::expect<toml::value>(data.at("General"));
        if (generalResult.is_ok()) {
            auto general = generalResult.unwrap();

            isNeo = toml::find_or<toml::boolean>(general, "isPS4Pro", false);
        }
    }
}
void save(const std::filesystem::path& path) {
    toml::basic_value<toml::preserve_comments> data;

    std::error_code error;
    if (std::filesystem::exists(path, error)) {
        try {
            data = toml::parse<toml::preserve_comments>(path);
        } catch (const std::exception& ex) {
            printf("Exception trying to parse config file. Exception: %s\n", ex.what());
            return;
        }
    } else {
        if (error) {
            printf("Filesystem error accessing %s (error: %s)\n", path.string().c_str(), error.message().c_str());
        }
        printf("Saving new configuration file %s\n", path.string().c_str());
    }

    data["General"]["isPS4Pro"] = isNeo;

    std::ofstream file(path, std::ios::out);
    file << data;
    file.close();
}
}  // namespace Config
