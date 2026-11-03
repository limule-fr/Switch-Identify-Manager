#include "localization.hpp"
#include "backup_manager.hpp"
#include "settings_manager.hpp"

#include <switch.h>

#include <cstdio>
#include <cstring>
#include <cerrno>
#include <string>
#include <sys/stat.h>

namespace
{
    constexpr int MENU_ITEM_COUNT = 5;

    void drawMenu(int selected, const Localization& localization)
    {
        consoleClear();
        std::printf("========================================\n");
        std::printf("       %s\n", localization.text("switch.ui_title").c_str());
        std::printf("========================================\n");
        std::printf("%s\n\n", localization.text("switch.ui_subtitle").c_str());

        const char* keys[MENU_ITEM_COUNT] = {
            "menu.diagnostic",
            "menu.prodinfo",
            "menu.backups",
            "menu.settings",
            "switch.exit"
        };
        for (int index = 0; index < MENU_ITEM_COUNT; ++index)
        {
            std::printf(
                "%c [%d] %s\n",
                index == selected ? '>' : ' ',
                index + 1,
                localization.text(keys[index]).c_str()
            );
        }

        std::printf("\n%s\n", localization.text("switch.ui_hint").c_str());
        std::printf("%s\n", localization.text("switch.no_nand_access").c_str());
        consoleUpdate(NULL);
    }

    void drawInformation(
        int selected,
        const Localization& localization,
        const std::string& settingsMessage
    )
    {
        consoleClear();
        std::printf("========================================\n");
        std::printf("%s\n\n", localization.text("switch.ui_title").c_str());

        if (selected == 0)
        {
            Result result = setsysInitialize();
            if (R_FAILED(result))
            {
                std::printf(
                    "%s: 0x%08X\n",
                    localization.text("switch.service_error").c_str(),
                    result
                );
            }
            else
            {
                SetSysFirmwareVersion firmware{};
                result = setsysGetFirmwareVersion(&firmware);
                if (R_SUCCEEDED(result))
                {
                    std::printf(
                        "%s: %.23s\n",
                        localization.text("switch.firmware").c_str(),
                        firmware.display_version
                    );
                }
                else
                {
                    std::printf(
                        "%s: 0x%08X\n",
                        localization.text("switch.firmware_error").c_str(),
                        result
                    );
                }
                setsysExit();
            }

            Result languageResult = setInitialize();
            if (R_SUCCEEDED(languageResult))
            {
                u64 languageCode = 0;
                languageResult = setGetSystemLanguage(&languageCode);
                if (R_SUCCEEDED(languageResult))
                {
                    char code[sizeof(languageCode) + 1] = {};
                    std::memcpy(code, &languageCode, sizeof(languageCode));
                    const std::string systemCode(code, strnlen(code, sizeof(code)));
                    std::printf(
                        "%s: %s\n",
                        localization.text("switch.system_language").c_str(),
                        localization.translateSystemLanguage(
                            systemCode.substr(0, 2)
                        ).c_str()
                    );
                }
                else
                {
                    std::printf(
                        "%s: 0x%08X\n",
                        localization.text("switch.language_error").c_str(),
                        languageResult
                    );
                }
                setExit();
            }
            else
            {
                std::printf(
                    "%s: 0x%08X\n",
                    localization.text("switch.language_error").c_str(),
                    languageResult
                );
            }
        }
        else if (selected == 1)
        {
            std::printf(
                "%s\n",
                localization.text("switch.prodinfo_unavailable").c_str()
            );
        }
        else if (selected == 2)
        {
            std::printf(
                "%s: /switch/SIM/backups\n\n",
                localization.text("switch.backup_path").c_str()
            );
            const BackupScanResult result = BackupManager(
                "sdmc:/switch/SIM/backups"
            ).scan();
            if (!result.error.empty())
            {
                std::printf(
                    "%s: %s\n",
                    localization.text("backup.error").c_str(),
                    localization.translate(result.error).c_str()
                );
            }
            else if (result.entries.empty())
            {
                std::printf(
                    "%s\n",
                    localization.text("backup.none").c_str()
                );
            }
            else
            {
                constexpr std::size_t MAX_DISPLAYED_BACKUPS = 6;
                const std::size_t displayCount =
                    result.entries.size() < MAX_DISPLAYED_BACKUPS
                        ? result.entries.size()
                        : MAX_DISPLAYED_BACKUPS;
                for (std::size_t index = 0; index < displayCount; ++index)
                {
                    const BackupEntry& entry = result.entries[index];
                    std::printf(
                        "%s (%llu %s) - %s\n",
                        entry.name.c_str(),
                        static_cast<unsigned long long>(entry.sizeBytes),
                        localization.text("backup.bytes").c_str(),
                        localization.text("backup.hash").c_str()
                    );
                    std::printf("  %s\n", entry.sha256.c_str()                    );
                    std::printf(
                        "  %s: %s\n",
                        localization.text("backup.integrity").c_str(),
                        localization.translate(entry.integrityMessage).c_str()
                    );
                }
                if (result.entries.size() > displayCount)
                {
                    std::printf(
                        "%s: %zu\n",
                        localization.text("backup.more_files").c_str(),
                        result.entries.size() - displayCount
                    );
                }
            }
            std::printf(
                "\n%s\n",
                localization.text("switch.sd_backup_notice").c_str()
            );
        }
        else if (selected == 3)
        {
            std::printf(
                "%s : %s\n\n",
                localization.text("settings.current_language").c_str(),
                localization.languageName().c_str()
            );
            std::printf(
                "%s\n",
                settingsMessage.c_str()
            );
            std::printf(
                "%s\n",
                localization.text("switch.no_nand_access").c_str()
            );
            std::printf(
                "\n%s\n",
                localization.text("switch.press_b").c_str()
            );
            consoleUpdate(NULL);
            return;
        }

        std::printf("\n%s\n", localization.text("switch.no_nand_access").c_str());
        std::printf("\n%s\n", localization.text("switch.press_b").c_str());
        consoleUpdate(NULL);
    }
}

int main()
{
    consoleInit(NULL);
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    PadState pad;
    padInitializeDefault(&pad);

    SettingsManager settingsManager("sdmc:/switch/SIM/sim.conf");
    const SettingsLoadResult settings = settingsManager.load();
    Localization localization(settings.language);
    int selected = 0;
    bool showInformation = false;
    bool running = true;
    std::string settingsMessage =
        localization.text("switch.language_changed");

    while (appletMainLoop() && running)
    {
        if (showInformation)
        {
            drawInformation(selected, localization, settingsMessage);
        }
        else
        {
            drawMenu(selected, localization);
        }

        padUpdate(&pad);
        const u64 keysDown = padGetButtonsDown(&pad);

        if (showInformation)
        {
            if (keysDown & HidNpadButton_B)
            {
                showInformation = false;
                settingsMessage = localization.text("switch.language_changed");
            }
        }
        else
        {
            if (keysDown & HidNpadButton_Up)
            {
                selected = (selected + MENU_ITEM_COUNT - 1) % MENU_ITEM_COUNT;
            }
            if (keysDown & HidNpadButton_Down)
            {
                selected = (selected + 1) % MENU_ITEM_COUNT;
            }
            if (keysDown & HidNpadButton_A)
            {
                if (selected == 3)
                {
                    Language nextLanguage = Language::FRENCH;
                    switch (localization.language())
                    {
                        case Language::FRENCH:
                            nextLanguage = Language::ENGLISH;
                            break;
                        case Language::ENGLISH:
                            nextLanguage = Language::SPANISH;
                            break;
                        case Language::SPANISH:
                            nextLanguage = Language::FRENCH;
                            break;
                    }

                    if (mkdir("sdmc:/switch/SIM", 0777) != 0
                        && errno != EEXIST)
                    {
                        settingsMessage =
                            localization.text("switch.settings_directory_error");
                        showInformation = true;
                    }
                    else
                    {
                        const SettingsSaveResult saved =
                            settingsManager.save(nextLanguage);
                        if (!saved.success)
                        {
                            settingsMessage =
                                localization.translate(saved.error);
                            showInformation = true;
                        }
                        else
                        {
                            localization = Localization(nextLanguage);
                            settingsMessage =
                                localization.text("switch.language_changed");
                            showInformation = true;
                        }
                    }
                }
                else if (selected == 4)
                {
                    running = false;
                }
                else
                {
                    showInformation = true;
                }
            }
            if (keysDown & HidNpadButton_Plus)
            {
                running = false;
            }
        }

        consoleUpdate(NULL);
        svcSleepThread(16'000'000);
    }

    consoleExit(NULL);
    return 0;
}
