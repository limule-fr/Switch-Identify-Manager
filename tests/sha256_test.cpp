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
                 / ("sim-sha256-" + std::to_string(stamp));
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
    const std::filesystem::path file = temporaryDirectory.getPath() / "data";

    {
        std::ofstream output(file, std::ios::binary);
        output << "abc";
    }
    const Sha256Result abc = sha256File(file);
    if (!abc.success
        || abc.digest
            != "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad")
    {
        std::cerr << "SHA-256 test vector for 'abc' failed.\n";
        return 1;
    }

    {
        std::ofstream output(file, std::ios::binary | std::ios::trunc);
    }
    const Sha256Result empty = sha256File(file);
    if (!empty.success
        || empty.digest
            != "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855")
    {
        std::cerr << "SHA-256 test vector for empty input failed.\n";
        return 1;
    }

    const Sha256Result missing = sha256File(temporaryDirectory.getPath() / "missing");
    if (missing.success || missing.error.empty())
    {
        std::cerr << "Missing-file error was not reported.\n";
        return 1;
    }

    std::cout << "All SHA-256 tests passed.\n";
    return 0;
}
