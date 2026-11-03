#include "localization.hpp"
#include "backup_manager.hpp"
#include "integrity_checker.hpp"
#include "prodinfo.hpp"
#include "report_generator.hpp"
#include "settings_manager.hpp"
#include "system_inspector.hpp"

#include <switch.h>

#include <cstdio>
#include <cstring>
#include <string>

namespace
{
    constexpr int MENU_ITEM_COUNT = 7;

    bool isSimpleFilename(const std::string& value)
    {
        return !value.empty()
            && value != "."
            && value != ".."
            && value.find('/') == std::string::npos
            && value.find('\\') == std::string::npos;
    }

    bool promptForPath(
        const char* header,
        std::string& value,
        const Localization& localization
    )
    {
        SwkbdConfig keyboard{};
        if (R_FAILED(swkbdCreate(&keyboard, 0)))
        {
            value = localization.text("switch.keyboard_error");
            return false;
        }

        swkbdConfigMakePresetDefault(&keyboard);
        swkbdConfigSetHeaderText(&keyboard, header);
        char buffer[512] = {};
        const Result result = swkbdShow(&keyboard, buffer, sizeof(buffer));
        swkbdClose(&keyboard);
        if (R_FAILED(result))
        {
            value = localization.text("switch.keyboard_error");
            return false;
        }

        value = buffer;
        return !value.empty();
    }

    void performBackupAction(
        bool create,
        BackupManager& backupManager,
        const Localization& localization,
        std::string& message
    )
    {
        std::string value;
        if (create)
        {
            if (!promptForPath(
                    localization.text("backup.source_prompt").c_str(),
                    value,
                    localization
                ))
            {
                if (!value.empty())
                {
                    message = value;
                }
                return;
            }

            const BackupOperationResult result =
                backupManager.createBackup(value);
            message = localization.translate(result.message);
            if (result.success)
            {
                message += " : " + result.path.string();
            }
            return;
        }

        if (!promptForPath(
                localization.text("backup.restore_name_prompt").c_str(),
                value,
                localization
            ))
        {
            if (!value.empty())
            {
                message = value;
            }
            return;
        }
        const std::string backupName = value;
        if (!promptForPath(
                localization.text("backup.restore_destination_prompt").c_str(),
                value,
                localization
            ))
        {
            if (!value.empty())
            {
                message = value;
            }
            return;
        }

        const std::string destination = value;
        std::printf(
            "%s: sdmc:/switch/SIM/backups/%s\n%s: %s\n",
            localization.text("backup.restore_source").c_str(),
            backupName.c_str(),
            localization.text("backup.restore_destination").c_str(),
            destination.c_str()
        );
        std::printf(
            "%s %s\n",
            localization.text("backup.restore_confirmation").c_str(),
            localization.text("restore.confirm_yes").c_str()
        );
        consoleUpdate(NULL);
        std::string confirmation;
        const std::string confirmationHeader =
            localization.text("backup.restore_confirmation") + " "
            + localization.text("restore.confirm_yes");
        if (!promptForPath(
                confirmationHeader.c_str(),
                confirmation,
                localization
            )
            || confirmation != localization.text("restore.confirm_yes"))
        {
            message = localization.text("backup.restore_cancelled");
            return;
        }

        const BackupOperationResult result =
            backupManager.restoreBackup(backupName, destination, true);
        message = localization.translate(result.message);
        if (result.success)
        {
            message += " : " + result.path.string();
        }
    }

    void performReportExport(
        const IntegrityChecker& integrityChecker,
        const SystemInspector& systemInspector,
        const BackupManager& backupManager,
        const ReportGenerator& reportGenerator,
        const Localization& localization,
        std::string& message
    )
    {
        std::string filename;
        if (!promptForPath(
                localization.text("switch.report_path_header").c_str(),
                filename,
                localization
            ))
        {
            if (!filename.empty())
            {
                message = filename;
            }
            return;
        }

        if (!isSimpleFilename(filename))
        {
            message = localization.text("message.invalid_input");
            return;
        }
        const SettingsSaveResult reportDirectory =
            SettingsManager(
                "sdmc:/switch/SIM/reports/SIM-report.txt"
            ).ensureParentDirectory();
        if (!reportDirectory.success)
        {
            message = localization.translate(reportDirectory.error);
            return;
        }
        const std::string destination =
            "sdmc:/switch/SIM/reports/" + filename;

        const std::string report = reportGenerator.generateText(
            integrityChecker.run(),
            systemInspector.run(),
            backupManager.scan(),
            localization
        );
        const ReportExportResult exported =
            reportGenerator.exportText(destination, report, false);
        if (!exported.success)
        {
            message = localization.text("report.export_error") + " : "
                    + localization.translate(exported.error);
            return;
        }
        message = localization.text("report.exported") + " : " + destination;
    }

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
            "menu.integrity",
            "menu.report",
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
        const std::string& settingsMessage,
        const IntegrityChecker& integrityChecker,
        const std::string& backupMessage,
        const std::string& reportMessage
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
            std::printf(
                "%s\n",
                localization.text("switch.backup_actions").c_str()
            );
            if (!backupMessage.empty())
            {
                std::printf("%s\n", backupMessage.c_str());
            }
        }
        else if (selected == 3)
        {
            const IntegrityReport report = integrityChecker.run();
            for (const IntegrityItem& item : report.items)
            {
                const char* status = "ERROR";
                switch (item.status)
                {
                    case CheckStatus::OK:
                        status = "OK";
                        break;
                    case CheckStatus::WARNING:
                        status = "WARNING";
                        break;
                    case CheckStatus::ERROR:
                        status = "ERROR";
                        break;
                    case CheckStatus::NOT_VERIFIED:
                        status = "NOT VERIFIED";
                        break;
                }
                std::printf(
                    "%s: %s (%d/%d)\n  %s\n",
                    localization.translate(item.name).c_str(),
                    localization.translate(status).c_str(),
                    item.points,
                    item.maxPoints,
                    localization.translate(item.message).c_str()
                );
            }
            std::printf(
                "\n%s: %d/100\n%s: %s\n",
                localization.text("integrity.score").c_str(),
                report.score,
                localization.text("integrity.general_status").c_str(),
                localization.translate(report.generalStatus).c_str()
            );
            std::printf(
                "%s: %d/100\n",
                localization.text("integrity.verified_coverage").c_str(),
                report.maximumVerifiedScore
            );
        }
        else if (selected == 4)
        {
            std::printf("%s\n", reportMessage.c_str());
        }
        else if (selected == 5)
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
    ProdInfoSimulator prodInfo;
    BackupManager backupManager("sdmc:/switch/SIM/backups");
    SystemInspector systemInspector;
    ReportGenerator reportGenerator;
    IntegrityChecker integrityChecker(
        prodInfo,
        backupManager,
        systemInspector,
        settingsManager
    );
    int selected = 0;
    bool showInformation = false;
    bool running = true;
    std::string backupMessage;
    std::string reportMessage;
    std::string settingsMessage =
        localization.text("switch.language_changed");

    while (appletMainLoop() && running)
    {
        if (showInformation)
        {
            drawInformation(
                selected,
                localization,
                settingsMessage,
                integrityChecker,
                backupMessage,
                reportMessage
            );
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
            else if (selected == 2 && (keysDown & HidNpadButton_X))
            {
                performBackupAction(
                    true,
                    backupManager,
                    localization,
                    backupMessage
                );
            }
            else if (selected == 2 && (keysDown & HidNpadButton_Y))
            {
                performBackupAction(
                    false,
                    backupManager,
                    localization,
                    backupMessage
                );
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
                if (selected == 5)
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

                    const SettingsSaveResult directoryResult =
                        settingsManager.ensureParentDirectory();
                    if (!directoryResult.success)
                    {
                        settingsMessage =
                            localization.translate(directoryResult.error);
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
                else if (selected == 6)
                {
                    running = false;
                }
                else if (selected == 4)
                {
                    performReportExport(
                        integrityChecker,
                        systemInspector,
                        backupManager,
                        reportGenerator,
                        localization,
                        reportMessage
                    );
                    showInformation = true;
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
