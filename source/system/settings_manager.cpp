#include "settings_manager.hpp"

#include <cerrno>
#include <fstream>
#include <sys/stat.h>
#include <utility>

namespace
{
    bool validateConfigDirectory(
        const std::filesystem::path& directory,
        bool allowMissing,
        bool& missing,
        std::string& errorMessage
    )
    {
        missing = false;
        for (const std::filesystem::path& component : directory)
        {
            if (component == "." || component == "..")
            {
                errorMessage = "Configuration path cannot contain . or ..";
                return false;
            }
        }
#ifdef __SWITCH__
        const std::string nativePath = directory.generic_string();
        if (nativePath.compare(0, 6, "sdmc:/") != 0)
        {
            errorMessage = "Configuration directory must be on the SD card.";
            return false;
        }
        struct stat rootStatus
        {
        };
        if (lstat("sdmc:/", &rootStatus) != 0
            || S_ISLNK(rootStatus.st_mode)
            || !S_ISDIR(rootStatus.st_mode))
        {
            errorMessage = "SD card root is unavailable or invalid.";
            return false;
        }
        std::string current = "sdmc:/";
        std::size_t offset = 6;
#else
        std::error_code absoluteError;
        const std::string nativePath =
            std::filesystem::absolute(directory, absoluteError)
                .lexically_normal()
                .generic_string();
        if (absoluteError)
        {
            errorMessage = "Could not resolve configuration directory.";
            return false;
        }
        std::string current = nativePath.empty() || nativePath.front() != '/'
            ? std::string()
            : std::string("/");
        std::size_t offset = current.empty() ? 0 : 1;
#endif
        bool missingComponent = false;
        while (offset < nativePath.size())
        {
            const std::size_t separator = nativePath.find('/', offset);
            const std::size_t end = separator == std::string::npos
                ? nativePath.size()
                : separator;
            const std::string component =
                nativePath.substr(offset, end - offset);
            offset = separator == std::string::npos
                ? nativePath.size()
                : separator + 1;
            if (component.empty())
            {
                continue;
            }
            if (component == "." || component == "..")
            {
                errorMessage = "Configuration path cannot contain . or ..";
                return false;
            }
            if (missingComponent)
            {
                continue;
            }
            if (!current.empty() && current.back() != '/')
            {
                current += '/';
            }
            current += component;

            struct stat status
            {
            };
            if (lstat(current.c_str(), &status) != 0)
            {
                if (errno == ENOENT && allowMissing)
                {
                    missing = true;
                    missingComponent = true;
                    continue;
                }
                errorMessage = "Could not inspect configuration directory.";
                return false;
            }
            if (S_ISLNK(status.st_mode))
            {
                errorMessage = "Configuration directory cannot contain symbolic links.";
                return false;
            }
            if (!S_ISDIR(status.st_mode))
            {
                errorMessage = "Configuration parent path is not a directory.";
                return false;
            }
        }
        return true;
    }

    SettingsSaveResult createConfigDirectory(
        const std::filesystem::path& directory
    )
    {
        for (const std::filesystem::path& component : directory)
        {
            if (component == "." || component == "..")
            {
                return {
                    false,
                    "Configuration path cannot contain . or .."
                };
            }
        }
#ifdef __SWITCH__
        const std::string nativePath = directory.generic_string();
        if (nativePath.compare(0, 6, "sdmc:/") != 0)
        {
            return {
                false,
                "Configuration directory must be on the SD card."
            };
        }
        struct stat rootStatus
        {
        };
        if (lstat("sdmc:/", &rootStatus) != 0
            || S_ISLNK(rootStatus.st_mode)
            || !S_ISDIR(rootStatus.st_mode))
        {
            return {false, "SD card root is unavailable or invalid."};
        }
        std::string current = "sdmc:/";
        std::size_t offset = 6;
#else
        std::error_code absoluteError;
        const std::string nativePath =
            std::filesystem::absolute(directory, absoluteError)
                .lexically_normal()
                .generic_string();
        if (absoluteError)
        {
            return {false, "Could not resolve configuration directory."};
        }
        std::string current = nativePath.empty() || nativePath.front() != '/'
            ? std::string()
            : std::string("/");
        std::size_t offset = current.empty() ? 0 : 1;
#endif
        while (offset < nativePath.size())
        {
            const std::size_t separator = nativePath.find('/', offset);
            const std::size_t end = separator == std::string::npos
                ? nativePath.size()
                : separator;
            const std::string component =
                nativePath.substr(offset, end - offset);
            offset = separator == std::string::npos
                ? nativePath.size()
                : separator + 1;
            if (component.empty())
            {
                continue;
            }
            if (component == "." || component == "..")
            {
                return {
                    false,
                    "Configuration path cannot contain . or .."
                };
            }
            if (!current.empty() && current.back() != '/')
            {
                current += '/';
            }
            current += component;

            struct stat status
            {
            };
            if (lstat(current.c_str(), &status) != 0)
            {
                if (errno != ENOENT && errno != ENOTDIR)
                {
                    return {
                        false,
                        "Could not inspect configuration directory."
                    };
                }
                if (mkdir(current.c_str(), 0777) != 0 && errno != EEXIST)
                {
                    return {
                        false,
                        "Could not create configuration directory."
                    };
                }
                if (lstat(current.c_str(), &status) != 0)
                {
                    return {
                        false,
                        "Could not inspect configuration directory."
                    };
                }
            }
            if (S_ISLNK(status.st_mode))
            {
                return {
                    false,
                    "Configuration directory cannot contain symbolic links."
                };
            }
            if (!S_ISDIR(status.st_mode))
            {
                return {
                    false,
                    "Configuration parent path is not a directory."
                };
            }
        }
        return {true, {}};
    }

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
    bool missingDirectory = false;
    std::string directoryError;
    const std::filesystem::path parent = configPath.parent_path().empty()
        ? std::filesystem::current_path()
        : configPath.parent_path();
    if (!validateConfigDirectory(
            parent,
            true,
            missingDirectory,
            directoryError
        ))
    {
        return {Language::FRENCH, directoryError, false};
    }
    if (missingDirectory)
    {
        return {Language::FRENCH, {}, false};
    }

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

SettingsSaveResult SettingsManager::ensureParentDirectory() const
{
    const std::filesystem::path parent = configPath.parent_path().empty()
        ? std::filesystem::current_path()
        : configPath.parent_path();
    return createConfigDirectory(parent);
}

SettingsSaveResult SettingsManager::save(Language language) const
{
    const SettingsSaveResult directoryResult = ensureParentDirectory();
    if (!directoryResult.success)
    {
        return directoryResult;
    }

    bool missingDirectory = false;
    std::string directoryError;
    const std::filesystem::path parent = configPath.parent_path().empty()
        ? std::filesystem::current_path()
        : configPath.parent_path();
    if (!validateConfigDirectory(
            parent,
            false,
            missingDirectory,
            directoryError
        ))
    {
        return {false, directoryError};
    }
    if (missingDirectory)
    {
        return {false, "Configuration directory does not exist."};
    }

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
