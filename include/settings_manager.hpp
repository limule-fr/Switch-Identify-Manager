#ifndef SETTINGS_MANAGER_HPP
#define SETTINGS_MANAGER_HPP

#include "localization.hpp"

#include <filesystem>
#include <string>
#include <utility>

struct SettingsLoadResult
{
    Language language;
    std::string warning;
    bool loadedFromFile;
};

struct SettingsSaveResult
{
    bool success;
    std::string error;
};

class SettingsManager
{
public:
    explicit SettingsManager(std::filesystem::path configPath = "sim.conf");

    SettingsLoadResult load() const;
    SettingsSaveResult ensureParentDirectory() const;
    SettingsSaveResult save(Language language) const;

private:
    std::filesystem::path configPath;
};

#endif
