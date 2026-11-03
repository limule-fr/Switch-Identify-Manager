
#include <iostream>
#include <filesystem>
#include <sstream>
#include <string>
#include <system_error>
#include "backup_manager.hpp"
#include "prodinfo.hpp"
#include "integrity_checker.hpp"
#include "logger.hpp"
#include "report_generator.hpp"
#include "settings_manager.hpp"
#include "system_inspector.hpp"

enum class InputResult
{
    VALID,
    INVALID,
    END_OF_INPUT
};

InputResult readInteger(int& value)
{
    std::string line;
    if (!std::getline(std::cin, line))
    {
        return InputResult::END_OF_INPUT;
    }

    std::istringstream input(line);
    if (!(input >> value))
    {
        return InputResult::INVALID;
    }

    input >> std::ws;
    return input.eof() ? InputResult::VALID : InputResult::INVALID;
}

void recordLog(Logger& logger, LogLevel level, const std::string& event)
{
    const LogResult result = logger.write(level, event);
    if (!result.success)
    {
        std::cerr << "Journalisation impossible : " << result.error << "\n";
    }
}

int main()
{
    ProdInfoSimulator prodInfo;
    BackupManager backupManager;
    SystemInspector systemInspector;
    ReportGenerator reportGenerator;
    SettingsManager settingsManager;
    IntegrityChecker integrityChecker(
        prodInfo,
        backupManager,
        systemInspector,
        settingsManager
    );
    Logger logger;
    const SettingsLoadResult settings = settingsManager.load();
    Localization localization(settings.language);
    recordLog(logger, LogLevel::INFO, "Desktop simulator started.");

    if (!settings.warning.empty())
    {
        recordLog(logger, LogLevel::WARNING, "Settings could not be loaded.");
        std::cerr << localization.text("settings.load_warning")
                  << ": " << localization.translate(settings.warning) << "\n";
    }

    while (true)
    {
        std::cout << "\n";
        std::cout << "====================================\n";
        std::cout << "       " << localization.text("app.title") << "\n";
        std::cout << "====================================\n\n";

        std::cout << "[1] " << localization.text("menu.diagnostic") << "\n";
        std::cout << "[2] " << localization.text("menu.prodinfo") << "\n";
        std::cout << "[3] " << localization.text("menu.backups") << "\n";
        std::cout << "[4] " << localization.text("menu.integrity") << "\n";
        std::cout << "[5] " << localization.text("menu.report") << "\n";
        std::cout << "[6] " << localization.text("menu.change_prodinfo") << "\n";
        std::cout << "[7] " << localization.text("menu.settings") << "\n";
        std::cout << "[8] " << localization.text("menu.create_backup") << "\n";
        std::cout << "[9] " << localization.text("menu.restore_backup") << "\n";
        std::cout << "[0] " << localization.text("menu.exit") << "\n\n";

        std::cout << localization.text("prompt.selection");

        int choice = -1;
        const InputResult inputResult = readInteger(choice);
        if (inputResult == InputResult::END_OF_INPUT)
        {
            std::cout << "\n" << localization.text("message.closing") << "\n";
            break;
        }

        if (inputResult == InputResult::INVALID)
        {
            recordLog(logger, LogLevel::WARNING, "Invalid menu input.");
            std::cout << "\n" << localization.text("message.invalid_input") << "\n";
            continue;
        }

        recordLog(
            logger,
            LogLevel::INFO,
            "Menu option selected: " + std::to_string(choice) + "."
        );

        switch (choice)
        {
            case 1:
                systemInspector.displayReport(localization);
                break;

            case 2:
                prodInfo.inspect(localization);
                break;

            case 3:
            {
                std::cout << "\n" << localization.text("backup.title") << "\n";
                const BackupScanResult result = backupManager.scan();
                if (!result.error.empty())
                {
                    recordLog(logger, LogLevel::ERROR, "Backup scan failed.");
                    std::cout << localization.text("backup.error") << " : "
                              << localization.translate(result.error) << "\n";
                    std::cout << localization.text("backup.add_files") << "\n";
                    break;
                }

                if (result.entries.empty())
                {
                    std::cout << localization.text("backup.none") << "\n";
                }

                for (const BackupEntry& entry : result.entries)
                {
                    std::cout << entry.name << " (" << entry.sizeBytes
                              << " " << localization.text("backup.bytes")
                              << ") : ";
                    switch (entry.status)
                    {
                        case BackupReadStatus::READABLE:
                            std::cout << localization.text("backup.readable");
                            break;

                        case BackupReadStatus::EMPTY:
                            std::cout << localization.text("backup.empty");
                            break;

                        case BackupReadStatus::ERROR:
                            std::cout << localization.text("backup.error");
                            break;
                    }
                    std::cout << " - " << localization.translate(entry.message)
                              << "\n";
                    std::cout << "  " << localization.text("backup.hash")
                              << " : " << entry.sha256 << "\n";

                    const char* integrityStatusKey =
                        "backup.integrity.not_verified";
                    switch (entry.integrityStatus)
                    {
                        case BackupIntegrityStatus::NOT_VERIFIED:
                            integrityStatusKey =
                                "backup.integrity.not_verified";
                            break;

                        case BackupIntegrityStatus::VERIFIED:
                            integrityStatusKey = "backup.integrity.verified";
                            break;

                        case BackupIntegrityStatus::MISMATCH:
                            integrityStatusKey = "backup.integrity.mismatch";
                            break;

                        case BackupIntegrityStatus::INVALID_CHECKSUM:
                            integrityStatusKey = "backup.integrity.invalid";
                            break;

                        case BackupIntegrityStatus::ERROR:
                            integrityStatusKey = "backup.integrity.error";
                            break;
                    }
                    std::cout << "  " << localization.text(integrityStatusKey)
                              << " : "
                              << localization.translate(entry.integrityMessage)
                              << "\n";
                }

                if (result.ignoredEntries > 0)
                {
                    std::cout << result.ignoredEntries
                              << " " << localization.text("backup.ignored")
                              << "\n";
                }
                std::cout << localization.text("backup.read_only") << "\n";
                std::cout << localization.text("backup.reference_help") << "\n";
                break;
            }

            case 4:
                integrityChecker.displayReport(localization);
                break;

            case 5:
            {
                const IntegrityReport integrityReport = integrityChecker.run();
                const SystemInspectionReport systemReport =
                    systemInspector.run();
                const BackupScanResult backupResult = backupManager.scan();
                const std::string report = reportGenerator.generateText(
                    integrityReport,
                    systemReport,
                    backupResult,
                    localization
                );

                std::cout << "\n" << localization.text("report.path");
                std::string outputPath;
                if (!std::getline(std::cin, outputPath))
                {
                    std::cout << "\n" << localization.text("message.closing") << "\n";
                    return 0;
                }
                if (outputPath.empty())
                {
                    outputPath = "SIM-report.txt";
                }

                bool overwrite = false;
                std::error_code pathError;
                const std::filesystem::file_status outputStatus =
                    std::filesystem::symlink_status(outputPath, pathError);
                if (!pathError && std::filesystem::exists(outputStatus))
                {
                    std::cout << localization.text("report.overwrite");
                    std::string confirmation;
                    if (!std::getline(std::cin, confirmation))
                    {
                        std::cout << "\n" << localization.text("message.closing") << "\n";
                        return 0;
                    }
                    const std::string yes =
                        localization.text("report.overwrite_yes");
                    overwrite = confirmation == yes
                             || confirmation == yes.substr(0, 1)
                             || confirmation == "oui"
                             || confirmation == "yes";
                }

                const ReportExportResult exportResult =
                    reportGenerator.exportText(outputPath, report, overwrite);
                if (!exportResult.success)
                {
                    recordLog(logger, LogLevel::ERROR, "Report export failed.");
                    std::cout << localization.text("report.export_error")
                              << " : "
                              << localization.translate(exportResult.error)
                              << "\n";
                }
                else
                {
                    recordLog(logger, LogLevel::INFO, "Report exported.");
                    std::cout << localization.text("report.exported") << " : "
                              << outputPath << "\n";
                }
                break;
            }

            case 6:
            {
                int stateChoice = -1;

                std::cout << "\n--- "
                          << localization.text("prodinfo.state_title")
                          << " ---\n";
                std::cout << "[1] " << localization.text("prodinfo.state_intact")
                          << "\n";
                std::cout << "[2] " << localization.text("prodinfo.state_missing")
                          << "\n";
                std::cout << "[3] "
                          << localization.text("prodinfo.state_corrupted")
                          << "\n";
                std::cout << "[4] " << localization.text("prodinfo.state_masked")
                          << "\n";
                std::cout << localization.text("prompt.selection");

                const InputResult stateInputResult = readInteger(stateChoice);
                if (stateInputResult == InputResult::END_OF_INPUT)
                {
                    std::cout << "\n" << localization.text("message.closing") << "\n";
                    return 0;
                }

                if (stateInputResult == InputResult::INVALID)
                {
                    std::cout << localization.text("message.invalid_input") << "\n";
                    break;
                }

                bool statusChanged = true;
                switch (stateChoice)
                {
                    case 1:
                        prodInfo.setStatus(ProdInfoStatus::INTACT);
                        break;

                    case 2:
                        prodInfo.setStatus(ProdInfoStatus::MISSING);
                        break;

                    case 3:
                        prodInfo.setStatus(ProdInfoStatus::CORRUPTED);
                        break;

                    case 4:
                        prodInfo.setStatus(ProdInfoStatus::MASKED);
                        break;

                    default:
                        std::cout << localization.text("message.unknown_option") << "\n";
                        statusChanged = false;
                        break;
                }

                if (statusChanged)
                {
                    std::cout << localization.text("prodinfo.state_changed")
                              << " : "
                              << prodInfo.getStatusName() << "\n";
                }
                break;
            }

            case 7:
            {
                std::cout << "\n========================================\n";
                std::cout << "           " << localization.text("settings.title")
                          << "\n";
                std::cout << "========================================\n";
                std::cout << localization.text("settings.current_language")
                          << " : " << localization.languageName() << "\n";
                std::cout << localization.text("settings.choose_language")
                          << "\n";
                std::cout << localization.text("prompt.selection");

                int languageChoice = 0;
                const InputResult languageInput = readInteger(languageChoice);
                if (languageInput == InputResult::END_OF_INPUT)
                {
                    std::cout << "\n" << localization.text("message.closing")
                              << "\n";
                    return 0;
                }
                if (languageInput == InputResult::INVALID)
                {
                    std::cout << localization.text("message.invalid_input")
                              << "\n";
                    break;
                }

                if (languageChoice == 0)
                {
                    break;
                }

                Language selectedLanguage = localization.language();
                switch (languageChoice)
                {
                    case 1:
                        selectedLanguage = Language::FRENCH;
                        break;
                    case 2:
                        selectedLanguage = Language::ENGLISH;
                        break;
                    case 3:
                        selectedLanguage = Language::SPANISH;
                        break;
                    default:
                        std::cout << localization.text("settings.invalid")
                                  << "\n";
                        break;
                }
                if (languageChoice < 1 || languageChoice > 3)
                {
                    break;
                }

                const SettingsSaveResult saveResult =
                    settingsManager.save(selectedLanguage);
                if (!saveResult.success)
                {
                    recordLog(logger, LogLevel::ERROR, "Language setting could not be saved.");
                    std::cout << localization.text("settings.save_error")
                              << ": "
                              << localization.translate(saveResult.error)
                              << "\n";
                    break;
                }

                if (selectedLanguage == localization.language())
                {
                    std::cout << localization.text("settings.unchanged")
                              << "\n";
                }
                else
                {
                    localization = Localization(selectedLanguage);
                    recordLog(logger, LogLevel::INFO, "Language setting changed.");
                    std::cout << localization.text("settings.saved") << "\n";
                }
                break;
            }

            case 8:
            {
                std::cout << localization.text("backup.source_prompt");
                std::string sourcePath;
                if (!std::getline(std::cin, sourcePath))
                {
                    std::cout << "\n" << localization.text("message.closing") << "\n";
                    return 0;
                }

                const BackupOperationResult result =
                    backupManager.createBackup(sourcePath);
                if (!result.success)
                {
                    recordLog(logger, LogLevel::ERROR, "Backup creation failed.");
                    std::cout << localization.text("backup.operation_error")
                              << " : " << localization.translate(result.message)
                              << "\n";
                }
                else
                {
                    recordLog(logger, LogLevel::INFO, "Backup created.");
                    std::cout << localization.translate(result.message)
                              << " : " << result.path.string() << "\n";
                }
                break;
            }

            case 9:
            {
                const BackupScanResult result = backupManager.scan();
                if (!result.error.empty())
                {
                    std::cout << localization.text("backup.error") << " : "
                              << localization.translate(result.error) << "\n";
                    break;
                }
                for (const BackupEntry& entry : result.entries)
                {
                    std::cout << "- " << entry.name << " : "
                              << localization.text(
                                  entry.integrityStatus
                                          == BackupIntegrityStatus::VERIFIED
                                      ? "backup.integrity.verified"
                                      : "backup.integrity.not_verified"
                              )
                              << "\n";
                }

                std::cout << localization.text("backup.restore_name_prompt");
                std::string backupName;
                if (!std::getline(std::cin, backupName))
                {
                    std::cout << "\n" << localization.text("message.closing") << "\n";
                    return 0;
                }
                std::cout << localization.text("backup.restore_destination_prompt");
                std::string destinationPath;
                if (!std::getline(std::cin, destinationPath))
                {
                    std::cout << "\n" << localization.text("message.closing") << "\n";
                    return 0;
                }

                const BackupOperationResult restored =
                    backupManager.restoreBackup(backupName, destinationPath);
                if (!restored.success)
                {
                    recordLog(logger, LogLevel::ERROR, "Backup restoration failed.");
                    std::cout << localization.text("backup.operation_error")
                              << " : "
                              << localization.translate(restored.message)
                              << "\n";
                }
                else
                {
                    recordLog(logger, LogLevel::INFO, "Backup restored.");
                    std::cout << localization.translate(restored.message)
                              << " : " << restored.path.string() << "\n";
                }
                break;
            }

            case 0:
                std::cout << "\n" << localization.text("message.closing") << "\n";
                break;

            default:
                recordLog(logger, LogLevel::WARNING, "Unknown menu option.");
                std::cout << "\n" << localization.text("message.unknown_option")
                          << "\n";
                break;
        }

        if (choice == 0)
        {
            break;
        }

        std::cout << "\n" << localization.text("prompt.continue");
        std::string line;
        if (!std::getline(std::cin, line))
        {
            std::cout << "\n" << localization.text("message.closing") << "\n";
            break;
        }
    }

    recordLog(logger, LogLevel::INFO, "Desktop simulator stopped.");
    return 0;
}