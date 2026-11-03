#include "backup_manager.hpp"

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
                 / ("sim-backup-manager-" + std::to_string(stamp));
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

    bool writeFile(
        const std::filesystem::path& path,
        const std::string& content
    )
    {
        std::ofstream file(path, std::ios::binary);
        file << content;
        return file.good();
    }
}

int main()
{
    TemporaryDirectory temporaryDirectory;
    const std::filesystem::path directory = temporaryDirectory.getPath();
    if (!writeFile(directory / "personal-save.dat", "abc")
        || !writeFile(directory / "empty-save.dat", ""))
    {
        std::cerr << "Could not create test backup files.\n";
        return 1;
    }
    if (!writeFile(
            directory / "personal-save.dat.sha256",
            "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad\n"
        ))
    {
        std::cerr << "Could not create reference checksum.\n";
        return 1;
    }
    std::filesystem::create_directory(directory / "nested");

    std::error_code symlinkError;
    std::filesystem::create_symlink(
        directory / "personal-save.dat",
        directory / "linked-save.dat",
        symlinkError
    );

    const BackupScanResult result = BackupManager(directory).scan();
    if (!result.error.empty())
    {
        std::cerr << "Unexpected scan error: " << result.error << "\n";
        return 1;
    }

    if (result.entries.size() != 2)
    {
        std::cerr << "Expected two regular backup files, got "
                  << result.entries.size() << ".\n";
        return 1;
    }

    const std::filesystem::path sourceDirectory = directory / "source-input";
    std::filesystem::create_directory(sourceDirectory);
    const std::filesystem::path sourcePath =
        sourceDirectory / "created-save.dat";
    if (!writeFile(sourcePath, "backup contents"))
    {
        std::cerr << "Could not create source file for backup operation tests.\n";
        return 1;
    }

    const BackupManager manager(directory);
    const BackupOperationResult created = manager.createBackup(sourcePath);
    if (!created.success
        || created.path != directory / "created-save.dat")
    {
        std::cerr << "Backup creation failed: " << created.message << "\n";
        return 1;
    }
    const BackupScanResult createdScan = manager.scan();
    bool createdBackupVerified = false;
    for (const BackupEntry& entry : createdScan.entries)
    {
        if (entry.name == "created-save.dat"
            && entry.integrityStatus == BackupIntegrityStatus::VERIFIED)
        {
            createdBackupVerified = true;
        }
    }
    if (!createdBackupVerified)
    {
        std::cerr << "New backup did not receive a valid SHA-256 reference.\n";
        return 1;
    }

    const std::filesystem::path restoreDirectory = directory / "restored";
    std::filesystem::create_directory(restoreDirectory);
    const std::filesystem::path restorePath =
        restoreDirectory / "restored-save.dat";
    const BackupOperationResult restored =
        manager.restoreBackup("created-save.dat", restorePath, true);
    if (!restored.success)
    {
        std::cerr << "Backup restoration failed: " << restored.message << "\n";
        return 1;
    }
    std::ifstream restoredFile(restorePath, std::ios::binary);
    std::string restoredContents;
    std::getline(restoredFile, restoredContents);
    if (restoredContents != "backup contents")
    {
        std::cerr << "Restored file contents did not match the backup.\n";
        return 1;
    }

    const BackupOperationResult refusedOverwrite =
        manager.restoreBackup("created-save.dat", restorePath, true);
    std::ifstream unchangedFile(restorePath, std::ios::binary);
    std::string unchangedContents;
    std::getline(unchangedFile, unchangedContents);
    if (refusedOverwrite.success || unchangedContents != "backup contents")
    {
        std::cerr << "Restore overwrote an existing destination.\n";
        return 1;
    }

    const BackupOperationResult refusedUnverified =
        manager.restoreBackup(
            "empty-save.dat",
            restoreDirectory / "empty.dat",
            true
        );
    if (refusedUnverified.success)
    {
        std::cerr << "Restore accepted a backup without a reference checksum.\n";
        return 1;
    }

    const BackupOperationResult refusedDuplicate =
        manager.createBackup(sourcePath);
    if (refusedDuplicate.success)
    {
        std::cerr << "Backup creation overwrote an existing backup.\n";
        return 1;
    }

    const std::filesystem::path unconfirmedPath =
        restoreDirectory / "unconfirmed.dat";
    const BackupOperationResult refusedUnconfirmed =
        manager.restoreBackup("created-save.dat", unconfirmedPath, false);
    if (refusedUnconfirmed.success
        || std::filesystem::exists(unconfirmedPath))
    {
        std::cerr << "Restore proceeded without explicit confirmation.\n";
        return 1;
    }

    const std::filesystem::path traversalPath =
        directory / "source-input" / ".." / "traversal.dat";
    const BackupOperationResult refusedTraversal =
        manager.restoreBackup("created-save.dat", traversalPath, true);
    if (refusedTraversal.success || std::filesystem::exists(
            directory / "traversal.dat"
        ))
    {
        std::cerr << "Restore accepted a path containing parent traversal.\n";
        return 1;
    }

    const std::filesystem::path linkedSourceDirectory =
        directory / "linked-source";
    std::error_code sourceParentLinkError;
    std::filesystem::create_directory_symlink(
        sourceDirectory,
        linkedSourceDirectory,
        sourceParentLinkError
    );
    if (!sourceParentLinkError
        && manager.createBackup(linkedSourceDirectory / "created-save.dat").success)
    {
        std::cerr << "Backup creation followed a symbolic-link parent.\n";
        return 1;
    }

    const std::filesystem::path linkedSourceFile =
        directory / "linked-source-file.dat";
    std::error_code sourceLinkError;
    std::filesystem::create_symlink(sourcePath, linkedSourceFile, sourceLinkError);
    if (!sourceLinkError
        && manager.createBackup(linkedSourceFile).success)
    {
        std::cerr << "Backup creation accepted a symbolic-link source.\n";
        return 1;
    }

    const std::filesystem::path linkedRestoreDirectory =
        directory / "linked-restore";
    std::error_code restoreParentLinkError;
    std::filesystem::create_directory_symlink(
        restoreDirectory,
        linkedRestoreDirectory,
        restoreParentLinkError
    );
    if (!restoreParentLinkError
        && manager.restoreBackup(
            "created-save.dat",
            linkedRestoreDirectory / "escaped.dat",
            true
        ).success)
    {
        std::cerr << "Restore followed a symbolic-link parent.\n";
        return 1;
    }

    const std::filesystem::path sandboxRoot = directory / "sandbox";
    const std::filesystem::path sandboxBackups = sandboxRoot / "backups";
    std::filesystem::create_directories(sandboxBackups);
    const std::filesystem::path sandboxSource =
        sourceDirectory / "verified.dat";
    if (!writeFile(sandboxSource, "verified contents"))
    {
        std::cerr << "Could not create sandbox backup fixture.\n";
        return 1;
    }
    const BackupManager sandboxManager(sandboxBackups);
    if (!sandboxManager.createBackup(sandboxSource).success)
    {
        std::cerr << "Could not create verified sandbox backup.\n";
        return 1;
    }
    const std::filesystem::path outsideRestoreDirectory =
        directory / "outside-root";
    std::filesystem::create_directory(outsideRestoreDirectory);
    const BackupOperationResult refusedOutsideRestore =
        sandboxManager.restoreBackup(
            "verified.dat",
            outsideRestoreDirectory / "escaped.dat",
            true
        );
    if (refusedOutsideRestore.success)
    {
        std::cerr << "Restore escaped its allowed SIM directory.\n";
        return 1;
    }

    const std::filesystem::path linkedBackupParent =
        directory / "linked-backup-parent";
    std::error_code backupParentLinkError;
    std::filesystem::create_directory_symlink(
        directory,
        linkedBackupParent,
        backupParentLinkError
    );
    if (!backupParentLinkError
        && BackupManager(linkedBackupParent / "backups").scan().error.empty())
    {
        std::cerr << "Backup scan followed a symbolic-link parent.\n";
        return 1;
    }

    bool foundReadable = false;
    bool foundEmpty = false;
    for (const BackupEntry& entry : result.entries)
    {
        if (entry.name == "personal-save.dat"
            && entry.sizeBytes == 3
            && entry.status == BackupReadStatus::READABLE
            && entry.sha256
                == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"
            && entry.integrityStatus == BackupIntegrityStatus::VERIFIED)
        {
            foundReadable = true;
        }
        if (entry.name == "empty-save.dat"
            && entry.sizeBytes == 0
            && entry.status == BackupReadStatus::EMPTY
            && entry.sha256
                == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
            && entry.integrityStatus == BackupIntegrityStatus::NOT_VERIFIED)
        {
            foundEmpty = true;
        }
    }

    if (!foundReadable || !foundEmpty)
    {
        std::cerr << "Backup hash, integrity status or file size was incorrect.\n";
        return 1;
    }

    if (!writeFile(directory / "personal-save.dat.sha256", std::string(64, '0')))
    {
        std::cerr << "Could not update reference checksum.\n";
        return 1;
    }
    const BackupScanResult mismatchResult = BackupManager(directory).scan();
    bool foundMismatch = false;
    for (const BackupEntry& entry : mismatchResult.entries)
    {
        if (entry.name == "personal-save.dat"
            && entry.integrityStatus == BackupIntegrityStatus::MISMATCH)
        {
            foundMismatch = true;
        }
    }
    if (!foundMismatch)
    {
        std::cerr << "Checksum mismatch was not detected.\n";
        return 1;
    }

    if (!writeFile(directory / "personal-save.dat.sha256", "invalid\n"))
    {
        std::cerr << "Could not write malformed checksum.\n";
        return 1;
    }
    const BackupScanResult invalidChecksumResult = BackupManager(directory).scan();
    bool foundInvalidChecksum = false;
    for (const BackupEntry& entry : invalidChecksumResult.entries)
    {
        if (entry.name == "personal-save.dat"
            && entry.integrityStatus == BackupIntegrityStatus::INVALID_CHECKSUM)
        {
            foundInvalidChecksum = true;
        }
    }
    if (!foundInvalidChecksum)
    {
        std::cerr << "Malformed checksum was not detected.\n";
        return 1;
    }

    const std::filesystem::path checksumTarget =
        directory / "checksum-target.txt";
    if (!writeFile(checksumTarget, "checksum target"))
    {
        std::cerr << "Could not create checksum symlink target.\n";
        return 1;
    }
    std::error_code checksumRemoveError;
    std::filesystem::remove(
        directory / "personal-save.dat.sha256",
        checksumRemoveError
    );
    if (checksumRemoveError)
    {
        std::cerr << "Could not remove malformed checksum fixture: "
                  << checksumRemoveError.message() << "\n";
        return 1;
    }
    std::error_code checksumSymlinkError;
    std::filesystem::create_symlink(
        checksumTarget,
        directory / "personal-save.dat.sha256",
        checksumSymlinkError
    );
    if (!checksumSymlinkError)
    {
        const BackupScanResult symlinkChecksumResult =
            BackupManager(directory).scan();
        bool rejectedChecksumLink = false;
        for (const BackupEntry& entry : symlinkChecksumResult.entries)
        {
            if (entry.name == "personal-save.dat"
                && entry.integrityStatus == BackupIntegrityStatus::ERROR)
            {
                rejectedChecksumLink = true;
            }
        }
        if (!rejectedChecksumLink)
        {
            std::cerr << "Backup scanner followed a checksum symbolic link.\n";
            return 1;
        }
    }

    const BackupScanResult missingResult =
        BackupManager(directory / "missing").scan();
    if (missingResult.error.empty())
    {
        std::cerr << "Missing backup directory was not reported.\n";
        return 1;
    }

    if (symlinkError)
    {
        std::cout << "Symlink test skipped: " << symlinkError.message() << "\n";
    }
    else if (result.ignoredEntries != 2)
    {
        std::cerr << "Expected nested directory and symlink to be ignored.\n";
        return 1;
    }

    std::cout << "All backup manager tests passed.\n";
    return 0;
}
