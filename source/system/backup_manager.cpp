#include "backup_manager.hpp"
#include "sha256.hpp"

#include <algorithm>
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

    std::string pathError(
        const std::string& context,
        const std::error_code& error
    )
    {
        return context + error.message();
    }

    bool prepareBackupDirectory(
        const std::filesystem::path& path,
        std::string& errorMessage
    )
    {
        std::error_code error;
        std::filesystem::file_status status =
            std::filesystem::symlink_status(path, error);
        if (error == std::errc::no_such_file_or_directory)
        {
            error.clear();
            if (!std::filesystem::create_directories(path, error) && error)
            {
                errorMessage = pathError(
                    "Impossible de creer le dossier de sauvegardes : ",
                    error
                );
                return false;
            }
            status = std::filesystem::symlink_status(path, error);
        }

        if (error)
        {
            errorMessage = pathError(
                "Impossible d'inspecter le dossier de sauvegardes : ",
                error
            );
            return false;
        }
        if (std::filesystem::is_symlink(status))
        {
            errorMessage =
                "Le dossier de sauvegardes ne peut pas etre un lien symbolique.";
            return false;
        }
        if (!std::filesystem::is_directory(status))
        {
            errorMessage = "Dossier de sauvegardes introuvable ou invalide.";
            return false;
        }
        return true;
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
        const std::filesystem::file_status checksumStatus =
            std::filesystem::symlink_status(checksumPath, checksumError);

        BackupIntegrityStatus integrityStatus =
            BackupIntegrityStatus::NOT_VERIFIED;
        std::string integrityMessage =
            "Aucune somme de reference; integrite non verifiee.";

        if (!checksumError && std::filesystem::is_symlink(checksumStatus))
        {
            integrityStatus = BackupIntegrityStatus::ERROR;
            integrityMessage =
                "Le fichier de somme ne peut pas etre un lien symbolique.";
        }
        else if (checksumError
                 && checksumError != std::errc::no_such_file_or_directory)
        {
            integrityStatus = BackupIntegrityStatus::ERROR;
            integrityMessage = "Impossible d'inspecter la somme de reference : "
                             + checksumError.message();
        }
        else if (!checksumError && std::filesystem::exists(checksumStatus))
        {
            if (!std::filesystem::is_regular_file(checksumStatus))
            {
                integrityStatus = BackupIntegrityStatus::ERROR;
                integrityMessage =
                    "La somme de reference n'est pas un fichier regulier.";
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

    bool removeCreatedFile(
        const std::filesystem::path& path,
        std::string& cleanupMessage
    )
    {
        std::error_code error;
        std::filesystem::remove(path, error);
        if (error)
        {
            cleanupMessage = " Nettoyage impossible : " + error.message();
            return false;
        }
        return true;
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
        result.error = pathError(
            "Impossible d'inspecter le dossier de sauvegardes : ",
            error
        );
        return result;
    }
    if (std::filesystem::is_symlink(directoryStatus))
    {
        result.error =
            "Le dossier de sauvegardes ne peut pas etre un lien symbolique.";
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
        result.error = pathError(
            "Impossible de parcourir le dossier de sauvegardes : ",
            error
        );
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
            if (backupError
                || !std::filesystem::is_regular_file(backupStatus))
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
            result.error = pathError(
                "Erreur pendant le parcours du dossier : ",
                error
            );
            break;
        }
    }
    return result;
}

BackupOperationResult BackupManager::createBackup(
    const std::filesystem::path& sourcePath
) const
{
    std::error_code error;
    const std::filesystem::file_status sourceStatus =
        std::filesystem::symlink_status(sourcePath, error);
    if (error)
    {
        return {
            false,
            pathError("Impossible d'inspecter le fichier source : ", error),
            {}
        };
    }
    if (std::filesystem::is_symlink(sourceStatus))
    {
        return {false, "La source ne peut pas etre un lien symbolique.", {}};
    }
    if (!std::filesystem::is_regular_file(sourceStatus))
    {
        return {false, "La source n'est pas un fichier regulier.", {}};
    }

    const std::string name = sourcePath.filename().string();
    if (name.empty() || name == "." || name == ".."
        || (name.size() >= 7
            && name.compare(name.size() - 7, 7, ".sha256") == 0))
    {
        return {false, "Nom de fichier source invalide.", {}};
    }

    std::string directoryError;
    if (!prepareBackupDirectory(backupDirectory, directoryError))
    {
        return {false, directoryError, {}};
    }

    const std::filesystem::path backupPath = backupDirectory / name;
    const std::filesystem::path checksumPath =
        backupPath.string() + ".sha256";
    const auto pathExists = [&error](
        const std::filesystem::path& path,
        bool& exists
    )
    {
        const std::filesystem::file_status status =
            std::filesystem::symlink_status(path, error);
        exists = !error && std::filesystem::exists(status);
        return !error || error == std::errc::no_such_file_or_directory;
    };

    bool exists = false;
    if (!pathExists(backupPath, exists))
    {
        return {
            false,
            pathError("Impossible d'inspecter la destination : ", error),
            backupPath
        };
    }
    if (exists)
    {
        return {
            false,
            "La sauvegarde existe deja; aucun fichier n'a ete remplace.",
            backupPath
        };
    }

    error.clear();
    if (!pathExists(checksumPath, exists))
    {
        return {
            false,
            pathError("Impossible d'inspecter la somme de reference : ", error),
            checksumPath
        };
    }
    if (exists)
    {
        return {false, "La somme de reference existe deja.", checksumPath};
    }

    error.clear();
    if (!std::filesystem::copy_file(
            sourcePath,
            backupPath,
            std::filesystem::copy_options::none,
            error
        ))
    {
        return {
            false,
            pathError("Impossible de creer la sauvegarde : ", error),
            backupPath
        };
    }

    const Sha256Result hash = sha256File(backupPath);
    if (!hash.success)
    {
        std::string cleanupMessage;
        removeCreatedFile(backupPath, cleanupMessage);
        return {
            false,
            "Impossible de calculer l'empreinte de la sauvegarde."
                + cleanupMessage,
            backupPath
        };
    }

    std::ofstream checksumFile(checksumPath, std::ios::out | std::ios::binary);
    if (!checksumFile)
    {
        std::string cleanupMessage;
        removeCreatedFile(backupPath, cleanupMessage);
        return {
            false,
            "Impossible d'ecrire la somme de reference de la sauvegarde."
                + cleanupMessage,
            checksumPath
        };
    }
    checksumFile << hash.digest << '\n';
    checksumFile.close();
    if (!checksumFile)
    {
        std::string cleanupMessage;
        removeCreatedFile(checksumPath, cleanupMessage);
        removeCreatedFile(backupPath, cleanupMessage);
        return {
            false,
            "Erreur pendant l'ecriture de la somme de reference."
                + cleanupMessage,
            checksumPath
        };
    }

    return {
        true,
        "Sauvegarde creee et somme SHA-256 enregistree.",
        backupPath
    };
}

BackupOperationResult BackupManager::restoreBackup(
    const std::string& backupName,
    const std::filesystem::path& destinationPath
) const
{
    const std::filesystem::path requestedName(backupName);
    if (backupName.empty()
        || requestedName.filename() != requestedName
        || (backupName.size() >= 7
            && backupName.compare(backupName.size() - 7, 7, ".sha256") == 0)
        || destinationPath.empty())
    {
        return {false, "Nom de sauvegarde invalide.", {}};
    }

    const BackupScanResult scanResult = scan();
    if (!scanResult.error.empty())
    {
        return {false, scanResult.error, {}};
    }
    const auto entry = std::find_if(
        scanResult.entries.begin(),
        scanResult.entries.end(),
        [&backupName](const BackupEntry& candidate)
        {
            return candidate.name == backupName;
        }
    );
    if (entry == scanResult.entries.end()
        || entry->status == BackupReadStatus::ERROR)
    {
        return {false, "Sauvegarde introuvable ou illisible.", {}};
    }
    if (entry->integrityStatus != BackupIntegrityStatus::VERIFIED)
    {
        return {
            false,
            "La sauvegarde doit avoir une somme de reference valide avant restauration.",
            {}
        };
    }

    std::error_code error;
    const std::filesystem::file_status destinationStatus =
        std::filesystem::symlink_status(destinationPath, error);
    if (!error && std::filesystem::exists(destinationStatus))
    {
        return {
            false,
            "La destination existe deja; aucun fichier n'a ete remplace.",
            destinationPath
        };
    }
    if (error != std::errc::no_such_file_or_directory && error)
    {
        return {
            false,
            pathError("Impossible d'inspecter la destination : ", error),
            destinationPath
        };
    }

    const std::filesystem::path parent = destinationPath.parent_path();
    if (!parent.empty())
    {
        error.clear();
        const std::filesystem::file_status parentStatus =
            std::filesystem::symlink_status(parent, error);
        if (error || !std::filesystem::is_directory(parentStatus))
        {
            return {
                false,
                error
                    ? pathError(
                        "Impossible d'inspecter le dossier de destination : ",
                        error
                    )
                    : "Le dossier de destination n'existe pas.",
                destinationPath
            };
        }
    }

    error.clear();
    const std::filesystem::path sourcePath = backupDirectory / backupName;
    if (!std::filesystem::copy_file(
            sourcePath,
            destinationPath,
            std::filesystem::copy_options::none,
            error
        ))
    {
        return {
            false,
            pathError("Impossible de restaurer la sauvegarde : ", error),
            destinationPath
        };
    }

    const Sha256Result restoredHash = sha256File(destinationPath);
    if (!restoredHash.success || restoredHash.digest != entry->sha256)
    {
        std::string cleanupMessage;
        removeCreatedFile(destinationPath, cleanupMessage);
        return {
            false,
            "La verification de la copie restauree a echoue."
                + cleanupMessage,
            destinationPath
        };
    }

    return {
        true,
        "Sauvegarde restauree apres verification SHA-256.",
        destinationPath
    };
}
