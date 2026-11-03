#include "settings_manager.hpp"

#include <fstream>
#include <utility>

namespace
{
    Language languageFromCode(const std::string& code, bool& valid)
    {
        valid = true;
        if (code == "fr")
        {
            return Language::FRENCH;
        }
        if (code == "en")
        {
            return Language::ENGLISH;
        }
        if (code == "es")
        {
            return Language::SPANISH;
        }

        valid = false;
        return Language::FRENCH;
    }
}

SettingsManager::SettingsManager(std::filesystem::path configPath)
    : configPath(std::move(configPath))
{
}

SettingsLoadResult SettingsManager::load() const
{
    std::error_code statusError;
    const std::filesystem::file_status status =
        std::filesystem::symlink_status(configPath, statusError);
    if (!statusError && std::filesystem::is_symlink(status))
    {
        return {
            Language::FRENCH,
            "Configuration file cannot be a symbolic link.",
            false
        };
    }

    std::ifstream input(configPath);
    if (!input)
    {
        std::error_code error;
        if (statusError == std::errc::no_such_file_or_directory
            || (!std::filesystem::exists(configPath, error) && !error))
        {
            return {Language::FRENCH, {}, false};
        }
        return {
            Language::FRENCH,
            "Could not read configuration file.",
            false
        };
    }

    std::string line;
    if (!std::getline(input, line) || line.rfind("language=", 0) != 0)
    {
        return {Language::FRENCH, "Invalid configuration format.", false};
    }

    bool valid = false;
    const Language language = languageFromCode(line.substr(9), valid);
    if (!valid)
    {
        return {Language::FRENCH, "Unsupported language code.", false};
    }

    std::string extraLine;
    if (std::getline(input, extraLine))
    {
        return {
            Language::FRENCH,
            "Unexpected extra configuration data.",
            false
        };
    }
    if (input.bad())
    {
        return {
            Language::FRENCH,
            "Error while reading configuration file.",
            false
        };
    }

    return {language, {}, true};
}

SettingsSaveResult SettingsManager::save(Language language) const
{
    std::error_code error;
    const std::filesystem::file_status status =
        std::filesystem::symlink_status(configPath, error);
    if (error == std::errc::no_such_file_or_directory)
    {
        error.clear();
    }
    else if (error)
    {
        return {
            false,
            "Could not inspect configuration file: " + error.message()
        };
    }
    else if (std::filesystem::is_symlink(status))
    {
        return {false, "Configuration file cannot be a symbolic link."};
    }
    else if (std::filesystem::exists(status)
             && !std::filesystem::is_regular_file(status))
    {
        return {false, "Configuration path is not a regular file."};
    }

    const Localization localization(language);
    std::ofstream output(configPath, std::ios::trunc);
    if (!output)
    {
        return {false, "Could not open configuration file for writing."};
    }

    output << "language=" << localization.languageCode() << '\n';
    output.close();
    if (!output)
    {
        return {false, "Error while writing configuration file."};
    }

    return {true, {}};
}
