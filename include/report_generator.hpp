#ifndef REPORT_GENERATOR_HPP
#define REPORT_GENERATOR_HPP

#include "backup_manager.hpp"
#include "integrity_checker.hpp"
#include "localization.hpp"
#include "system_inspector.hpp"

#include <filesystem>
#include <string>

struct ReportExportResult
{
    bool success;
    std::string error;
};

class ReportGenerator
{
public:
    std::string generateText(
        const IntegrityReport& integrityReport,
        const SystemInspectionReport& systemReport,
        const BackupScanResult& backupResult,
        const Localization& localization = Localization()
    ) const;

    ReportExportResult exportText(
        const std::filesystem::path& outputPath,
        const std::string& content,
        bool overwrite = false
    ) const;
};

#endif
