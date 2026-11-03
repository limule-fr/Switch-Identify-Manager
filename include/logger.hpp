#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <filesystem>
#include <string>

enum class LogLevel
{
    INFO,
    WARNING,
    ERROR
};

struct LogResult
{
    bool success;
    std::string error;
};

class Logger
{
public:
    explicit Logger(std::filesystem::path logPath = "logs/sim.log");

    LogResult write(LogLevel level, const std::string& event) const;

private:
    std::filesystem::path logPath;
};

#endif
