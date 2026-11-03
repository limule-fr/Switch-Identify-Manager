#ifndef SHA256_HPP
#define SHA256_HPP

#include <filesystem>
#include <string>

struct Sha256Result
{
    bool success;
    std::string digest;
    std::string error;
};

Sha256Result sha256File(const std::filesystem::path& path);

#endif
