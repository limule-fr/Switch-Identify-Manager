#include "logger.hpp"

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
                 / ("sim-logger-" + std::to_string(stamp));
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
}

int main()
{
    TemporaryDirectory temporaryDirectory;
    const std::filesystem::path logPath =
        temporaryDirectory.getPath() / "logs" / "sim.log";
    const Logger logger(logPath);

    const LogResult infoResult = logger.write(LogLevel::INFO, "started");
    const LogResult warningResult = logger.write(
        LogLevel::WARNING,
        "invalid input\nsecond line"
    );
    if (!infoResult.success || !warningResult.success)
    {
        std::cerr << "Logger failed to append an entry.\n";
        return 1;
    }

    std::ifstream input(logPath, std::ios::binary);
    const std::string contents(
        (std::istreambuf_iterator<char>(input)),
        std::istreambuf_iterator<char>()
    );
    if (contents.find("[INFO] started\n") == std::string::npos
        || contents.find("[WARNING] invalid input second line\n")
               == std::string::npos)
    {
        std::cerr << "Logger output format or newline sanitization failed.\n";
        return 1;
    }

    const std::filesystem::path invalidPath =
        temporaryDirectory.getPath() / "file-as-parent" / "sim.log";
    {
        std::ofstream blocker(temporaryDirectory.getPath() / "file-as-parent");
        blocker << "not a directory";
    }
    const LogResult failure = Logger(invalidPath).write(LogLevel::ERROR, "event");
    if (failure.success || failure.error.empty())
    {
        std::cerr << "Logger did not surface a filesystem error.\n";
        return 1;
    }

    const std::filesystem::path symlinkTarget =
        temporaryDirectory.getPath() / "target.txt";
    {
        std::ofstream target(symlinkTarget);
        target << "keep";
    }
    const std::filesystem::path symlinkLog =
        temporaryDirectory.getPath() / "linked.log";
    std::error_code symlinkError;
    std::filesystem::create_symlink(symlinkTarget, symlinkLog, symlinkError);
    if (!symlinkError)
    {
        const LogResult refused =
            Logger(symlinkLog).write(LogLevel::INFO, "must not append");
        if (refused.success)
        {
            std::cerr << "Logger followed a symbolic link.\n";
            return 1;
        }

        std::ifstream target(symlinkTarget);
        std::string content;
        std::getline(target, content);
        if (content != "keep")
        {
            std::cerr << "Symbolic-link target was unexpectedly changed.\n";
            return 1;
        }
    }

    std::cout << "All logger tests passed.\n";
    return 0;
}
