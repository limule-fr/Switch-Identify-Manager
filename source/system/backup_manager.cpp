#include "backup_manager.hpp"
#include "sha256.hpp"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <fcntl.h>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
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

    bool isWithinPath(
        const std::filesystem::path& path,
        const std::filesystem::path& root
    )
    {
        std::string candidate;
        std::string allowed;
#ifdef __SWITCH__
        candidate = path.lexically_normal().generic_string();
        allowed = root.lexically_normal().generic_string();
#else
        std::error_code error;
        candidate = std::filesystem::absolute(path, error)
                        .lexically_normal()
                        .generic_string();
        if (error)
        {
            return false;
        }
        allowed = std::filesystem::absolute(root, error)
                      .lexically_normal()
                      .generic_string();
        if (error)
        {
            return false;
        }
#endif
        if (candidate == allowed)
        {
            return true;
        }
        if (allowed.empty() || allowed.back() != '/')
        {
            allowed += '/';
        }
        return candidate.compare(0, allowed.size(), allowed) == 0;
    }

    bool hasNoSymlinkComponents(
        const std::filesystem::path& path,
        bool allowMissing,
        std::string& errorMessage
    )
    {
        for (const std::filesystem::path& component : path)
        {
            if (component == "." || component == "..")
            {
                errorMessage = "Les chemins contenant . ou .. sont refuses.";
                return false;
            }
        }
#ifdef __SWITCH__
        std::string nativePath = path.generic_string();
        if (nativePath.compare(0, 6, "sdmc:/") != 0)
        {
            errorMessage = "Le chemin doit se trouver sur la carte SD.";
            return false;
        }
        struct stat rootStatus
        {
        };
        if (lstat("sdmc:/", &rootStatus) != 0
            || S_ISLNK(rootStatus.st_mode)
            || !S_ISDIR(rootStatus.st_mode))
        {
            errorMessage = "La carte SD est indisponible ou invalide.";
            return false;
        }
        std::string current = "sdmc:/";
        std::size_t offset = 6;
#else
        std::error_code absoluteError;
        const std::filesystem::path absolutePath =
            std::filesystem::absolute(path, absoluteError);
        if (absoluteError)
        {
            errorMessage = pathError(
                "Impossible de normaliser le chemin : ",
                absoluteError
            );
            return false;
        }
        const std::string nativePath = absolutePath.generic_string();
        std::string current = nativePath.empty() || nativePath.front() != '/'
            ? std::string()
            : std::string("/");
        std::size_t offset = current.empty() ? 0 : 1;
#endif
        bool missingComponent = false;
        while (offset < nativePath.size())
        {
            const std::size_t separator = nativePath.find('/', offset);
            const std::size_t end = separator == std::string::npos
                ? nativePath.size()
                : separator;
            const std::string component =
                nativePath.substr(offset, end - offset);
            offset = separator == std::string::npos
                ? nativePath.size()
                : separator + 1;
            if (component.empty())
            {
                continue;
            }
            if (component == "." || component == "..")
            {
                errorMessage = "Les chemins contenant . ou .. sont refuses.";
                return false;
            }
            if (missingComponent)
            {
                continue;
            }
            if (!current.empty() && current.back() != '/')
            {
                current += '/';
            }
            current += component;

            struct stat status
            {
            };
            if (lstat(current.c_str(), &status) != 0)
            {
                if (allowMissing && errno == ENOENT)
                {
                    missingComponent = true;
                    continue;
                }
                errorMessage = "Impossible d'inspecter un composant du chemin.";
                return false;
            }
            if (S_ISLNK(status.st_mode))
            {
                errorMessage =
                    "Les liens symboliques sont refuses dans les chemins sensibles.";
                return false;
            }
        }
        return true;
    }

    bool prepareBackupDirectory(
        const std::filesystem::path& path,
        std::string& errorMessage
    )
    {
        if (!hasNoSymlinkComponents(path, true, errorMessage))
        {
            return false;
        }
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
        if (!hasNoSymlinkComponents(path, false, errorMessage))
        {
            return false;
        }
        return true;
    }

    bool writeFileExclusively(
        const std::filesystem::path& path,
        const std::string& contents,
        std::string& errorMessage
    )
    {
        int flags = O_WRONLY | O_CREAT | O_EXCL;
#ifdef O_NOFOLLOW
        flags |= O_NOFOLLOW;
#endif
        const int descriptor = open(path.c_str(), flags, S_IRUSR | S_IWUSR);
        if (descriptor < 0)
        {
            errorMessage = std::error_code(errno, std::generic_category()).message();
            return false;
        }

        std::size_t offset = 0;
        while (offset < contents.size())
        {
            const ssize_t written = write(
                descriptor,
                contents.data() + offset,
                contents.size() - offset
            );
            if (written <= 0)
            {
                const int writeError = errno;
                close(descriptor);
                if (unlink(path.c_str()) != 0)
                {
                    errorMessage = "Could not remove partial checksum: "
                        + std::error_code(errno, std::generic_category()).message();
                }
                else
                {
                    errorMessage =
                        std::error_code(writeError, std::generic_category()).message();
                }
                return false;
            }
            offset += static_cast<std::size_t>(written);
        }
        if (close(descriptor) != 0)
        {
            const int closeError = errno;
            if (unlink(path.c_str()) != 0)
            {
                errorMessage = "Could not remove partial checksum: "
                    + std::error_code(errno, std::generic_category()).message();
            }
            else
            {
                errorMessage =
                    std::error_code(closeError, std::generic_category()).message();
            }
            return false;
        }
        return true;
    }

    bool copyFileExclusively(
        const std::filesystem::path& sourcePath,
        const std::filesystem::path& destinationPath,
        std::string& errorMessage
    )
    {
        int sourceFlags = O_RDONLY;
#ifdef O_NOFOLLOW
        sourceFlags |= O_NOFOLLOW;
#endif
        const int source = open(sourcePath.c_str(), sourceFlags);
        if (source < 0)
        {
            errorMessage =
                std::error_code(errno, std::generic_category()).message();
            return false;
        }

        struct stat sourceStatus
        {
        };
        if (fstat(source, &sourceStatus) != 0)
        {
            const int statError = errno;
            close(source);
            errorMessage =
                std::error_code(statError, std::generic_category()).message();
            return false;
        }
        if (!S_ISREG(sourceStatus.st_mode))
        {
            close(source);
            errorMessage = "Source is not a regular file.";
            return false;
        }

        int destinationFlags = O_WRONLY | O_CREAT | O_EXCL;
#ifdef O_NOFOLLOW
        destinationFlags |= O_NOFOLLOW;
#endif
        const int destination = open(
            destinationPath.c_str(),
            destinationFlags,
            S_IRUSR | S_IWUSR
        );
        if (destination < 0)
        {
            const int openError = errno;
            close(source);
            errorMessage =
                std::error_code(openError, std::generic_category()).message();
            return false;
        }

        char buffer[8192];
        bool success = true;
        while (true)
        {
            const ssize_t count = read(source, buffer, sizeof(buffer));
            if (count == 0)
            {
                break;
            }
            if (count < 0)
            {
                errorMessage =
                    std::error_code(errno, std::generic_category()).message();
                success = false;
                break;
            }

            ssize_t writtenTotal = 0;
            while (writtenTotal < count)
            {
                const ssize_t written = write(
                    destination,
                    buffer + writtenTotal,
                    static_cast<std::size_t>(count - writtenTotal)
                );
                if (written <= 0)
                {
                    errorMessage =
                        std::error_code(errno, std::generic_category()).message();
                    success = false;
                    break;
                }
                writtenTotal += written;
            }
            if (!success)
            {
                break;
            }
        }

        const int sourceCloseResult = close(source);
        const int sourceCloseError = sourceCloseResult == 0 ? 0 : errno;
        const int destinationCloseResult = close(destination);
        const int destinationCloseError =
            destinationCloseResult == 0 ? 0 : errno;
        if (success
            && (sourceCloseError != 0 || destinationCloseError != 0))
        {
            errorMessage = std::error_code(
                sourceCloseError != 0
                    ? sourceCloseError
                    : destinationCloseError,
                std::generic_category()
            ).message();
            success = false;
        }
        if (!success)
        {
            if (unlink(destinationPath.c_str()) != 0)
            {
                errorMessage += "; partial destination could not be removed: "
                    + std::error_code(errno, std::generic_category()).message();
            }
        }
        return success;
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
    std::string pathValidationError;
    if (!hasNoSymlinkComponents(
            backupDirectory,
            false,
            pathValidationError
        ))
    {
        result.error = pathValidationError;
        return result;
    }

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
    std::string pathValidationError;
    if (!hasNoSymlinkComponents(sourcePath, false, pathValidationError))
    {
        return {false, pathValidationError, {}};
    }
#ifdef __SWITCH__
    if (!isWithinPath(sourcePath, "sdmc:/"))
    {
        return {false, "La source doit se trouver sur la carte SD.", {}};
    }
#endif

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
    const std::filesystem::path checksumPath = backupPath.string() + ".sha256";
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

    std::string copyError;
    if (!copyFileExclusively(sourcePath, backupPath, copyError))
    {
        return {
            false,
            "Impossible de creer la sauvegarde : " + copyError,
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

    std::string checksumWriteError;
    if (!writeFileExclusively(
            checksumPath,
            hash.digest + '\n',
            checksumWriteError
        ))
    {
        std::string cleanupMessage;
        removeCreatedFile(backupPath, cleanupMessage);
        return {
            false,
            "Impossible d'ecrire la somme de reference de la sauvegarde : "
                + checksumWriteError
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
    const std::filesystem::path& destinationPath,
    bool confirmed
) const
{
    if (!confirmed)
    {
        return {false, "Restauration non confirmee.", destinationPath};
    }

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

    std::string pathValidationError;
    if (!hasNoSymlinkComponents(
            backupDirectory / backupName,
            false,
            pathValidationError
        ))
    {
        return {false, pathValidationError, {}};
    }
    if (!hasNoSymlinkComponents(
            destinationPath.parent_path().empty()
                ? std::filesystem::current_path()
                : destinationPath.parent_path(),
            false,
            pathValidationError
        ))
    {
        return {false, pathValidationError, destinationPath};
    }
    const std::filesystem::path allowedRestoreRoot =
        backupDirectory.parent_path().empty()
            ? std::filesystem::path(".")
            : backupDirectory.parent_path();
    if (!isWithinPath(destinationPath, allowedRestoreRoot))
    {
        return {
            false,
            "La restauration doit rester dans le dossier SIM autorise.",
            destinationPath
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

    const std::filesystem::path sourcePath = backupDirectory / backupName;
    std::string copyError;
    if (!copyFileExclusively(sourcePath, destinationPath, copyError))
    {
        return {
            false,
            "Impossible de restaurer la sauvegarde : " + copyError,
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
