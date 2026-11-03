#include "report_generator.hpp"

#include <fstream>
#include <system_error>

namespace
{
    const char* checkStatusName(CheckStatus status)
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

}

std::string ReportGenerator::generateText(
    const IntegrityReport& integrityReport,
    const SystemInspectionReport& systemReport,
    const BackupScanResult& backupResult,
    const Localization& localization
) const
{
    std::string report;
    report += localization.text("report.title") + "\n";
    report += localization.text("diagnostic.mode") + " : "
            + localization.translate(systemReport.mode) + "\n";
    report += localization.text("report.score_notice") + "\n";
    if (systemReport.mode == "SIMULATEUR PC")
    {
        report += localization.text("report.simulated_notice") + "\n\n";
    }
    else
    {
        report += localization.text("report.source") + " : "
                + localization.translate(systemReport.mode) + ".\n\n";
    }

    report += localization.text("report.integrity") + "\n";
    report += localization.text("report.score") + " : "
            + std::to_string(integrityReport.score) + "/100\n";
    report += localization.text("report.overall") + " : "
            + localization.translate(integrityReport.generalStatus) + "\n";
    for (const IntegrityItem& item : integrityReport.items)
    {
        report += "- " + localization.translate(item.name) + " : ";
        report += localization.translate(checkStatusName(item.status));
        report += " (" + std::to_string(item.points) + "/"
                + std::to_string(item.maxPoints) + ") - "
                + localization.translate(item.message) + "\n";
    }

    report += "\n" + localization.text("report.system") + "\n";
    for (const SystemInfoItem& item : systemReport.items)
    {
        report += "- " + localization.translate(item.name) + " : "
                + localization.translate(item.value) + "\n";
    }

    report += "\n" + localization.text("report.backups") + "\n";
    if (!backupResult.error.empty())
    {
        report += localization.text("report.scan_error") + " : "
                + localization.translate(backupResult.error) + "\n";
    }
    else if (backupResult.entries.empty())
    {
        report += localization.text("report.no_backups") + "\n";
    }
    else
    {
        for (const BackupEntry& entry : backupResult.entries)
        {
            report += "- " + entry.name + " (" + std::to_string(entry.sizeBytes)
                    + " " + localization.text("backup.bytes") + ") : ";
            report += localization.text(
                entry.status == BackupReadStatus::READABLE
                    ? "backup.readable"
                    : entry.status == BackupReadStatus::EMPTY
                        ? "backup.empty"
                        : "backup.error"
            );
            report += " - " + localization.translate(entry.message) + "\n";
            report += "  " + localization.text("backup.hash") + " : "
                    + entry.sha256 + "\n";
            const char* integrityStatusKey = "backup.integrity.not_verified";
            switch (entry.integrityStatus)
            {
                case BackupIntegrityStatus::NOT_VERIFIED:
                    integrityStatusKey = "backup.integrity.not_verified";
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
            report += "  " + localization.text(integrityStatusKey) + " : "
                    + localization.translate(entry.integrityMessage) + "\n";
        }
    }
    report += localization.text("report.read_check") + "\n";
    report += localization.text("backup.reference_help") + "\n";
    if (backupResult.error.empty() && backupResult.ignoredEntries > 0)
    {
        report += localization.text("report.ignored") + " : "
                + std::to_string(backupResult.ignoredEntries) + "\n";
    }

    return report;
}

ReportExportResult ReportGenerator::exportText(
    const std::filesystem::path& outputPath,
    const std::string& content,
    bool overwrite
) const
{
    if (outputPath.empty())
    {
        return {false, "Le chemin de sortie est vide."};
    }

    std::error_code error;
    const std::filesystem::file_status outputStatus =
        std::filesystem::symlink_status(outputPath, error);
    if (error == std::errc::no_such_file_or_directory)
    {
        error.clear();
    }
    else if (error)
    {
        return {
            false,
            "Impossible d'inspecter le fichier de sortie : " + error.message()
        };
    }
    else if (std::filesystem::is_symlink(outputStatus))
    {
        return {false, "L'export vers un lien symbolique est refuse."};
    }
    else if (std::filesystem::exists(outputStatus))
    {
        if (!std::filesystem::is_regular_file(outputStatus))
        {
            return {false, "Le chemin de sortie n'est pas un fichier regulier."};
        }
        if (!overwrite)
        {
            return {
                false,
                "Le fichier existe deja; confirmez son remplacement."
            };
        }
    }

    std::ofstream output(outputPath, std::ios::binary | std::ios::trunc);
    if (!output)
    {
        return {false, "Impossible d'ouvrir le fichier de rapport en ecriture."};
    }

    output << content;
    output.close();
    if (!output)
    {
        return {false, "Erreur pendant l'ecriture du rapport."};
    }

    return {true, {}};
}
