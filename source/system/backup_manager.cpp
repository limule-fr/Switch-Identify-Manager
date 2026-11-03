#include "backup_manager.hpp"
#include "sha256.hpp"

#include <cctype>
#include <fstream>
#include <string>
#include <utility>

namespace
{
    bool isSha256(const std::string& digest)
    {
        if (digest.size() != 64)
        {
            return false;
        }

        for (const unsigned char character : digest)
        {
            if (!std::isxdigit(character))
            {
                return false;
            }
        }
        return true;
    }

    std::string lowercase(std::string value)
    {
        for (char& character : value)
        {
            character = static_cast<char>(
                std::tolower(static_cast<unsigned char>(character))
            );
        }
        return value;
    }

    BackupEntry inspectFile(
        const std::filesystem::directory_entry& entry,
        const std::string& name
    )
    {
        std::error_code error;
        const std::uintmax_t size = entry.file_size(error);
        if (error)
        {
            return {
                name,
                0,
                BackupReadStatus::ERROR,
                "Impossible de lire la taille du fichier : " + error.message(),
                {},
                BackupIntegrityStatus::ERROR,
                "Impossible de lire la taille du fichier."
            };
        }

        const Sha256Result hash = sha256File(entry.path());
        if (!hash.success)
        {
            return {
                name,
                size,
                BackupReadStatus::ERROR,
                "Fichier inaccessible ou erreur de lecture.",
                {},
                BackupIntegrityStatus::ERROR,
                hash.error
            };
        }

        const std::filesystem::path checksumPath =
            entry.path().string() + ".sha256";
        std::error_code checksumError;
        const std::filesystem::file_status checksumFileStatus =
            std::filesystem::symlink_status(checksumPath, checksumError);

        BackupIntegrityStatus integrityStatus =
            BackupIntegrityStatus::NOT_VERIFIED;
        std::string integrityMessage =
            "Aucune somme de reference; integrite non verifiee.";

        if (!checksumError && std::filesystem::is_symlink(checksumFileStatus))
        {
            integrityStatus = BackupIntegrityStatus::ERROR;
            integrityMessage = "Le fichier de somme ne peut pas etre un lien symbolique.";
        }
        else if (checksumError
                 && checksumError != std::errc::no_such_file_or_directory)
        {
            integrityStatus = BackupIntegrityStatus::ERROR;
            integrityMessage = "Impossible d'inspecter la somme de reference : "
                             + checksumError.message();
        }
        else if (!checksumError
                 && std::filesystem::exists(checksumFileStatus))
        {
            if (!std::filesystem::is_regular_file(checksumFileStatus))
            {
                integrityStatus = BackupIntegrityStatus::ERROR;
                integrityMessage = "La somme de reference n'est pas un fichier regulier.";
            }
            else
            {
                std::ifstream checksumFile(checksumPath);
                std::string expected;
                std::string extra;
                if (!(checksumFile >> expected)
                    || (checksumFile >> extra)
                    || !isSha256(expected))
                {
                    integrityStatus = BackupIntegrityStatus::INVALID_CHECKSUM;
                    integrityMessage =
                        "Le fichier .sha256 doit contenir une seule somme SHA-256 de 64 caracteres hexadecimaux.";
                }
                else if (lowercase(expected) == hash.digest)
                {
                    integrityStatus = BackupIntegrityStatus::VERIFIED;
                    integrityMessage = "La somme SHA-256 correspond.";
                }
                else
                {
                    integrityStatus = BackupIntegrityStatus::MISMATCH;
                    integrityMessage = "La somme SHA-256 ne correspond pas.";
                }
            }
        }

        if (size == 0)
        {
            return {
                name,
                size,
                BackupReadStatus::EMPTY,
                "Le fichier est vide.",
                hash.digest,
                integrityStatus,
                integrityMessage
            };
        }

        return {
            name,
            size,
            BackupReadStatus::READABLE,
            "Lecture complete.",
            hash.digest,
            integrityStatus,
            integrityMessage
        };
    }
}

BackupManager::BackupManager(std::filesystem::path backupDirectory)
    : backupDirectory(std::move(backupDirectory))
{
}

BackupScanResult BackupManager::scan() const
{
    BackupScanResult result;
    std::error_code error;
    const std::filesystem::file_status directoryStatus =
        std::filesystem::symlink_status(backupDirectory, error);

    if (error)
    {
        result.error = "Impossible d'inspecter le dossier de sauvegardes : "
                     + error.message();
        return result;
    }

    if (std::filesystem::is_symlink(directoryStatus))
    {
        result.error = "Le dossier de sauvegardes ne peut pas etre un lien symbolique.";
        return result;
    }

    if (!std::filesystem::is_directory(directoryStatus))
    {
        result.error = "Dossier de sauvegardes introuvable ou invalide.";
        return result;
    }

    std::filesystem::directory_iterator iterator(backupDirectory, error);
    if (error)
    {
        result.error = "Impossible de parcourir le dossier de sauvegardes : "
                     + error.message();
        return result;
    }

    const std::filesystem::directory_iterator end;
    while (iterator != end)
    {
        const std::filesystem::directory_entry entry = *iterator;
        const std::string name = entry.path().filename().string();
        const std::filesystem::file_status entryStatus =
            entry.symlink_status(error);

        if (error)
        {
            result.entries.push_back({
                name,
                0,
                BackupReadStatus::ERROR,
                "Impossible d'inspecter l'entree : " + error.message(),
                {},
                BackupIntegrityStatus::ERROR,
                "Impossible d'inspecter l'entree."
            });
            error.clear();
        }
        else if (!std::filesystem::is_regular_file(entryStatus))
        {
            ++result.ignoredEntries;
        }
        else if (name.size() >= 7
                 && name.compare(name.size() - 7, 7, ".sha256") == 0)
        {
            std::error_code backupError;
            const std::filesystem::file_status backupStatus =
                std::filesystem::symlink_status(
                    entry.path().parent_path()
                        / name.substr(0, name.size() - 7),
                    backupError
                );
            if (backupError || !std::filesystem::is_regular_file(backupStatus))
            {
                ++result.ignoredEntries;
            }
        }
        else
        {
            result.entries.push_back(inspectFile(entry, name));
        }

        iterator.increment(error);
        if (error)
        {
            result.error = "Erreur pendant le parcours du dossier : "
                         + error.message();
            break;
        }
    }

    return result;
}
