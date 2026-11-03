#ifndef INTEGRITY_CHECKER_HPP
#define INTEGRITY_CHECKER_HPP

#include "localization.hpp"
#include "prodinfo.hpp"
#include "backup_manager.hpp"
#include "system_inspector.hpp"
#include "settings_manager.hpp"

#include <string>
#include <vector>

enum class CheckStatus
{
    OK,
    WARNING,
    ERROR
};

struct IntegrityItem
{
    std::string name;
    CheckStatus status;
    int points;
    int maxPoints;
    std::string message;
};

struct IntegrityReport
{
    int score;
    std::string generalStatus;
    std::vector<IntegrityItem> items;
};

class IntegrityChecker
{
public:
    IntegrityChecker(
        const ProdInfoSimulator& prodInfo,
        const BackupManager& backupManager,
        const SystemInspector& systemInspector,
        const SettingsManager& settingsManager
    );

    IntegrityReport run() const;
    void displayReport(
        const Localization& localization = Localization()
    ) const;

private:
    const ProdInfoSimulator& prodInfo;
    const BackupManager& backupManager;
    const SystemInspector& systemInspector;
    const SettingsManager& settingsManager;

    std::string statusToString(CheckStatus status) const;
};

#endif