#include "integrity_checker.hpp"

#include <iostream>

IntegrityChecker::IntegrityChecker(
    const ProdInfoSimulator& prodInfo,
    const BackupManager& backupManager,
    const SystemInspector& systemInspector,
    const SettingsManager& settingsManager
)
    : prodInfo(prodInfo),
      backupManager(backupManager),
      systemInspector(systemInspector),
      settingsManager(settingsManager)
{
}

IntegrityReport IntegrityChecker::run() const
{
    IntegrityReport report{};

#ifdef __SWITCH__
    report.items.push_back({
        "PRODINFO",
        CheckStatus::WARNING,
        0,
        40,
        "Inspection PRODINFO indisponible; aucune donnee sensible n'est lue."
    });
#else
    switch (prodInfo.getStatus())
    {
        case ProdInfoStatus::INTACT:
            report.items.push_back({
                "PRODINFO",
                CheckStatus::OK,
                40,
                40,
                "Donnees simulees coherentes."
            });
            break;

        case ProdInfoStatus::MASKED:
            report.items.push_back({
                "PRODINFO",
                CheckStatus::WARNING,
                25,
                40,
                "Donnees simulees masquees."
            });
            break;

        case ProdInfoStatus::MISSING:
            report.items.push_back({
                "PRODINFO",
                CheckStatus::ERROR,
                0,
                40,
                "Donnees simulees absentes."
            });
            break;

        case ProdInfoStatus::CORRUPTED:
            report.items.push_back({
                "PRODINFO",
                CheckStatus::ERROR,
                0,
                40,
                "Anomalie simulee detectee."
            });
            break;
    }
#endif

    const SystemInspectionReport systemReport = systemInspector.run();
    report.items.push_back({
        "SYSTEM DATA",
        systemReport.inspectionAvailable
            ? CheckStatus::OK
            : CheckStatus::WARNING,
        systemReport.inspectionAvailable ? 25 : 0,
        25,
        systemReport.inspectionAvailable
            ? "Informations systeme lues; aucun controle d'integrite materielle n'est effectue."
            : "Diagnostic materiel indisponible dans le simulateur PC."
    });

    const BackupScanResult backupResult = backupManager.scan();
    int verifiedBackups = 0;
    bool allBackupsVerified = !backupResult.entries.empty()
                           && backupResult.error.empty();
    for (const BackupEntry& entry : backupResult.entries)
    {
        if (entry.status == BackupReadStatus::ERROR
            || entry.integrityStatus != BackupIntegrityStatus::VERIFIED)
        {
            allBackupsVerified = false;
        }
        else
        {
            ++verifiedBackups;
        }
    }

    const int backupPoints = allBackupsVerified
        ? 20
        : backupResult.entries.empty()
            ? 0
            : verifiedBackups * 20
                / static_cast<int>(backupResult.entries.size());
    const CheckStatus backupStatus = !backupResult.error.empty()
        ? CheckStatus::ERROR
        : allBackupsVerified
            ? CheckStatus::OK
            : backupResult.entries.empty() || backupPoints > 0
                ? CheckStatus::WARNING
                : CheckStatus::ERROR;
    report.items.push_back({
        "BACKUP",
        backupStatus,
        backupPoints,
        20,
        !backupResult.error.empty()
            ? "Verification des sauvegardes impossible."
            : backupResult.entries.empty()
                ? "Aucune sauvegarde disponible."
                : allBackupsVerified
                    ? "Toutes les sauvegardes ont une somme SHA-256 valide."
                    : "Certaines sauvegardes ne sont pas verifiees."
    });

    const SettingsLoadResult settings = settingsManager.load();
    const bool settingsValid = settings.loadedFromFile
                            && settings.warning.empty();
    report.items.push_back({
        "CONFIGURATION",
        settingsValid ? CheckStatus::OK : CheckStatus::WARNING,
        settingsValid ? 15 : 0,
        15,
        settingsValid
            ? "Configuration valide chargee depuis le fichier."
            : settings.warning.empty()
                ? "Aucun fichier de configuration persistant."
                : "Configuration invalide ou illisible."
    });

    for (const IntegrityItem& item : report.items)
    {
        report.score += item.points;
    }

    if (report.score == 100)
    {
        report.generalStatus = "EXCELLENT";
    }
    else if (report.score >= 75)
    {
        report.generalStatus = "BON";
    }
    else if (report.score >= 50)
    {
        report.generalStatus = "DEGRADE";
    }
    else
    {
        report.generalStatus = "CRITIQUE";
    }

    return report;
}

std::string IntegrityChecker::statusToString(
    CheckStatus status
) const
{
    switch (status)
    {
        case CheckStatus::OK:
            return "OK";

        case CheckStatus::WARNING:
            return "WARNING";

        case CheckStatus::ERROR:
            return "ERROR";
    }

    return "UNKNOWN";
}

void IntegrityChecker::displayReport(const Localization& localization) const
{
    const IntegrityReport report = run();

    std::cout << "\n";
    std::cout << "========================================\n";
    std::cout << "           " << localization.text("integrity.title") << "\n";
    std::cout << "========================================\n\n";

    std::cout << localization.text("integrity.mode") << "\n";
    std::cout << localization.text("integrity.disclaimer") << "\n\n";

    for (const IntegrityItem& item : report.items)
    {
        std::cout << localization.translate(item.name)
                  << " : "
                  << localization.translate(statusToString(item.status))
                  << "\n";

        std::cout << "  " << localization.translate(item.message) << "\n";
        std::cout << "  " << localization.text("integrity.points") << " : "
                  << item.points << "/"
                  << item.maxPoints << "\n\n";
    }

    std::cout << "----------------------------------------\n";
    std::cout << localization.text("integrity.score") << " : "
              << report.score << "/100\n";
    std::cout << localization.text("integrity.general_status") << " : "
              << localization.translate(report.generalStatus) << "\n";
    std::cout << "========================================\n";
}
