#include "integrity_checker.hpp"
#include "sha256.hpp"

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
                 / ("sim-integrity-checker-" + std::to_string(stamp));
            std::filesystem::create_directories(path / "backups");
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

    bool checkSimulatedProdInfo(
        ProdInfoStatus prodInfoStatus,
        int expectedScore,
        const std::string& expectedGeneralStatus,
        CheckStatus expectedCheckStatus
    )
    {
        TemporaryDirectory temporaryDirectory;
        ProdInfoSimulator prodInfo;
        prodInfo.setStatus(prodInfoStatus);
        BackupManager backupManager(temporaryDirectory.getPath() / "backups");
        SystemInspector systemInspector;
        SettingsManager settingsManager(
            temporaryDirectory.getPath() / "sim.conf"
        );

        const IntegrityReport report = IntegrityChecker(
            prodInfo,
            backupManager,
            systemInspector,
            settingsManager
        ).run();
        bool passed = true;

        if (report.score != expectedScore)
        {
            std::cerr << "Expected score " << expectedScore
                      << ", got " << report.score << ".\n";
            passed = false;
        }
        if (report.generalStatus != expectedGeneralStatus)
        {
            std::cerr << "Expected general status " << expectedGeneralStatus
                      << ", got " << report.generalStatus << ".\n";
            passed = false;
        }
        if (report.items.size() != 4)
        {
            std::cerr << "Expected 4 integrity items, got "
                      << report.items.size() << ".\n";
            return false;
        }
        if (report.items.front().status != expectedCheckStatus)
        {
            std::cerr << "Unexpected PRODINFO check status.\n";
            passed = false;
        }
        if (report.items[1].points != 0
            || report.items[1].status != CheckStatus::WARNING)
        {
            std::cerr << "Unavailable PC system diagnostics received points.\n";
            passed = false;
        }
        int totalMaximumPoints = 0;
        for (const IntegrityItem& item : report.items)
        {
            totalMaximumPoints += item.maxPoints;
            if (item.points < 0 || item.points > item.maxPoints)
            {
                std::cerr << "Invalid points for " << item.name << ".\n";
                passed = false;
            }
        }
        if (totalMaximumPoints != 100)
        {
            std::cerr << "Expected a maximum score of 100, got "
                      << totalMaximumPoints << ".\n";
            passed = false;
        }
        return passed;
    }

    bool checkRealModuleResults()
    {
        TemporaryDirectory temporaryDirectory;
        const std::filesystem::path backupDirectory =
            temporaryDirectory.getPath() / "backups";
        const std::filesystem::path backupPath =
            backupDirectory / "verified.dat";
        {
            std::ofstream backup(backupPath, std::ios::binary);
            backup << "abc";
        }
        {
            std::ofstream checksum(
                backupPath.string() + ".sha256",
                std::ios::binary
            );
            checksum
                << "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad\n";
        }

        SettingsManager settingsManager(
            temporaryDirectory.getPath() / "sim.conf"
        );
        if (!settingsManager.save(Language::ENGLISH).success)
        {
            std::cerr << "Could not save settings for integrity test.\n";
            return false;
        }

        ProdInfoSimulator prodInfo;
        BackupManager backupManager(backupDirectory);
        SystemInspector systemInspector;
        auto report = IntegrityChecker(
            prodInfo,
            backupManager,
            systemInspector,
            settingsManager
        ).run();
        if (report.score != 75
            || report.items[2].points != 20
            || report.items[3].points != 15)
        {
            std::cerr << "Integrity score did not reflect verified backup and settings.\n";
            return false;
        }

        {
            std::ofstream unverified(backupDirectory / "unverified.dat");
            unverified << "not verified";
        }
        report = IntegrityChecker(
            prodInfo,
            backupManager,
            systemInspector,
            settingsManager
        ).run();
        if (report.score != 65 || report.items[2].points != 10)
        {
            std::cerr << "Unverified backup did not reduce the integrity score.\n";
            return false;
        }
        return true;
    }
}

int main()
{
    bool passed = true;
    passed &= checkSimulatedProdInfo(
        ProdInfoStatus::INTACT, 40, "CRITIQUE", CheckStatus::OK
    );
    passed &= checkSimulatedProdInfo(
        ProdInfoStatus::MASKED, 25, "CRITIQUE", CheckStatus::WARNING
    );
    passed &= checkSimulatedProdInfo(
        ProdInfoStatus::MISSING, 0, "CRITIQUE", CheckStatus::ERROR
    );
    passed &= checkSimulatedProdInfo(
        ProdInfoStatus::CORRUPTED, 0, "CRITIQUE", CheckStatus::ERROR
    );
    passed &= checkRealModuleResults();

    if (!passed)
    {
        return 1;
    }

    std::cout << "All integrity checker tests passed.\n";
    return 0;
}
