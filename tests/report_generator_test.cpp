#include "report_generator.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
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
                 / ("sim-report-generator-" + std::to_string(stamp));
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

    bool contains(const std::string& text, const std::string& part)
    {
        return text.find(part) != std::string::npos;
    }
}

int main()
{
    IntegrityReport integrityReport{
        85,
        "BON",
        {
            {
                "PRODINFO",
                CheckStatus::WARNING,
                25,
                40,
                "Donnees simulees masquees."
            }
        }
    };
    SystemInspectionReport systemReport{
        "SIMULATEUR PC",
        false,
        {
            {"Acces NAND", "Non effectue"},
            {"Acces PRODINFO", "Non effectue"}
        }
    };
    BackupScanResult backupResult;
    backupResult.entries.push_back({
        "personal-save.dat",
        123,
        BackupReadStatus::READABLE,
        "Lecture complete.",
        "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
        BackupIntegrityStatus::VERIFIED,
        "La somme SHA-256 correspond."
    });

    const ReportGenerator generator;
    const std::string report =
        generator.generateText(integrityReport, systemReport, backupResult);
    if (!contains(report, "Score : 85/100")
        || !contains(report, "PRODINFO : AVERTISSEMENT")
        || !contains(report, "personal-save.dat (123 octets)")
        || !contains(report, "Acces NAND : Non effectue")
        || !contains(report, "ne garantit pas l'acces aux services Nintendo")
        || !contains(report, "SHA-256 est calcule")
        || !contains(report, "VERIFIEE : La somme SHA-256 correspond.")
        || !contains(
            report,
            "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"
        ))
    {
        std::cerr << "Generated report is missing expected content.\n";
        return 1;
    }

    const std::string englishReport = generator.generateText(
        integrityReport,
        systemReport,
        backupResult,
        Localization(Language::ENGLISH)
    );
    const std::string spanishReport = generator.generateText(
        integrityReport,
        systemReport,
        backupResult,
        Localization(Language::SPANISH)
    );
    if (!contains(englishReport, "INTEGRITY CHECK")
        || !contains(englishReport, "Score : 85/100")
        || !contains(spanishReport, "COMPROBACION DE INTEGRIDAD")
        || !contains(spanishReport, "Puntuacion : 85/100"))
    {
        std::cerr << "Report translations are incomplete.\n";
        return 1;
    }

    TemporaryDirectory temporaryDirectory;
    const std::filesystem::path outputPath =
        temporaryDirectory.getPath() / "report.txt";
    const ReportExportResult exportResult =
        generator.exportText(outputPath, report);
    if (!exportResult.success)
    {
        std::cerr << "Could not export report: " << exportResult.error << "\n";
        return 1;
    }

    std::ifstream input(outputPath, std::ios::binary);
    const std::string written(
        (std::istreambuf_iterator<char>(input)),
        std::istreambuf_iterator<char>()
    );
    if (written != report)
    {
        std::cerr << "Exported report content does not match.\n";
        return 1;
    }

    const ReportExportResult refusedOverwrite =
        generator.exportText(outputPath, "replacement");
    if (refusedOverwrite.success || refusedOverwrite.error.empty())
    {
        std::cerr << "Existing report was overwritten without confirmation.\n";
        return 1;
    }

    const ReportExportResult confirmedOverwrite =
        generator.exportText(outputPath, "replacement", true);
    if (!confirmedOverwrite.success)
    {
        std::cerr << "Confirmed overwrite failed: "
                  << confirmedOverwrite.error << "\n";
        return 1;
    }

    std::cout << "All report generator tests passed.\n";
    return 0;
}
