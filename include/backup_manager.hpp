#ifndef BACKUP_MANAGER_HPP
#define BACKUP_MANAGER_HPP

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

enum class BackupReadStatus
{
    READABLE,
    EMPTY,
    ERROR
};

enum class BackupIntegrityStatus
{
    NOT_VERIFIED,
    VERIFIED,
    MISMATCH,
    INVALID_CHECKSUM,
    ERROR
};

struct BackupEntry
{
    std::string name;
    std::uintmax_t sizeBytes;
    BackupReadStatus status;
    std::string message;
    std::string sha256;
    BackupIntegrityStatus integrityStatus;
    std::string integrityMessage;
};

struct BackupScanResult
{
    std::vector<BackupEntry> entries;
    std::size_t ignoredEntries = 0;
    std::string error;
};

struct BackupOperationResult
{
    bool success;
    std::string message;
    std::filesystem::path path;
};

class BackupManager
{
public:
    explicit BackupManager(std::filesystem::path backupDirectory = "backups");

    BackupScanResult scan() const;
    BackupOperationResult createBackup(
        const std::filesystem::path& sourcePath
    ) const;
    BackupOperationResult restoreBackup(
        const std::string& backupName,
        const std::filesystem::path& destinationPath
    ) const;

private:
    std::filesystem::path backupDirectory;
};

#endif
