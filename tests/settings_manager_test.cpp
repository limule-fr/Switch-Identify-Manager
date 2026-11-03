#include "settings_manager.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace
{
    class TemporaryDirectory
    {
    public:
        TemporaryDirectory()
        {
            const auto stamp = std::chrono::steady_clock::now()
                                   .time_since_epoch()
                                   .count();
            path = std::filesystem::temp_directory_path()
                 / ("sim-settings-" + std::to_string(stamp));
            std::filesystem::create_directories(path);
        }

        ~TemporaryDirectory()
        {
            std::error_code error;
            std::filesystem::remove_all(path, error);
        }

        const std::filesystem::path& getPath() const
        {
            return path;
        }

    private:
        std::filesystem::path path;
    };
}

int main()
{
    TemporaryDirectory temporaryDirectory;
    const std::filesystem::path config =
        temporaryDirectory.getPath() / "sim.conf";
    const SettingsManager settings(config);

    const SettingsLoadResult defaults = settings.load();
    if (defaults.language != Language::FRENCH
        || !defaults.warning.empty()
        || defaults.loadedFromFile)
    {
        std::cerr << "Missing config should default to French without warning.\n";
        return 1;
    }

    for (const Language language : {
             Language::FRENCH,
             Language::ENGLISH,
             Language::SPANISH
         })
    {
        const SettingsSaveResult saved = settings.save(language);
        if (!saved.success)
        {
            std::cerr << "Could not save settings: " << saved.error << "\n";
            return 1;
        }
        const SettingsLoadResult loaded = settings.load();
        if (!loaded.warning.empty()
            || loaded.language != language
            || !loaded.loadedFromFile)
        {
            std::cerr << "Saved language did not round-trip.\n";
            return 1;
        }
    }

    {
        std::ofstream invalid(config, std::ios::trunc);
        invalid << "language=de\n";
    }
    const SettingsLoadResult invalidSettings = settings.load();
    if (invalidSettings.language != Language::FRENCH
        || invalidSettings.warning.empty()
        || invalidSettings.loadedFromFile)
    {
        std::cerr << "Unsupported language should report a warning.\n";
        return 1;
    }

    const Localization english(Language::ENGLISH);
    const Localization spanish(Language::SPANISH);
    if (english.text("menu.settings") != "Settings"
        || spanish.text("menu.settings") != "Configuracion")
    {
        std::cerr << "Language catalog returned incorrect menu translations.\n";
        return 1;
    }

    const std::filesystem::path target =
        temporaryDirectory.getPath() / "outside.txt";
    {
        std::ofstream output(target);
        output << "language=en\n";
    }
    const std::filesystem::path linkedConfig =
        temporaryDirectory.getPath() / "linked.conf";
    std::error_code symlinkError;
    std::filesystem::create_symlink(target, linkedConfig, symlinkError);
    if (!symlinkError)
    {
        const SettingsManager linkedSettings(linkedConfig);
        const SettingsSaveResult refusedSave =
            linkedSettings.save(Language::SPANISH);
        if (refusedSave.success || refusedSave.error.empty())
        {
            std::cerr << "Settings save followed a symbolic link.\n";
            return 1;
        }

        const SettingsLoadResult refusedLoad = linkedSettings.load();
        if (refusedLoad.warning.empty())
        {
            std::cerr << "Settings load accepted a symbolic link.\n";
            return 1;
        }

        std::ifstream input(target);
        std::string targetContents;
        std::getline(input, targetContents);
        if (targetContents != "language=en")
        {
            std::cerr << "Symbolic-link target was unexpectedly changed.\n";
            return 1;
        }
    }

    std::cout << "All settings tests passed.\n";
    return 0;
}
