#include "logger.hpp"

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <system_error>
#include <utility>

namespace
{
    const char* levelName(LogLevel level)
    {
        switch (level)
        {
            case LogLevel::INFO:
                return "INFO";
            case LogLevel::WARNING:
                return "WARNING";
            case LogLevel::ERROR:
                return "ERROR";
        }

        return "UNKNOWN";
    }

    std::string timestamp()
    {
        const std::time_t now = std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now()
        );
        const std::tm* localTime = std::localtime(&now);
        if (localTime == nullptr)
        {
            return "unknown-time";
        }

        std::ostringstream formatted;
        formatted << std::put_time(localTime, "%Y-%m-%d %H:%M:%S");
        return formatted.str();
    }
}

Logger::Logger(std::filesystem::path logPath)
    : logPath(std::move(logPath))
{
}

LogResult Logger::write(LogLevel level, const std::string& event) const
{
    if (logPath.empty())
    {
        return {false, "Log path is empty."};
    }

    std::error_code error;
    const std::filesystem::path parent = logPath.parent_path();
    if (!parent.empty())
    {
        std::filesystem::create_directories(parent, error);
        if (error)
        {
            return {
                false,
                "Could not create log directory: " + error.message()
            };
        }
    }

    const std::filesystem::file_status logStatus =
        std::filesystem::symlink_status(logPath, error);
    if (error == std::errc::no_such_file_or_directory)
    {
        error.clear();
    }
    else if (error)
    {
        return {false, "Could not inspect log file: " + error.message()};
    }
    else if (std::filesystem::is_symlink(logStatus))
    {
        return {false, "Log file cannot be a symbolic link."};
    }
    else if (std::filesystem::exists(logStatus)
             && !std::filesystem::is_regular_file(logStatus))
    {
        return {false, "Log path is not a regular file."};
    }

    std::ofstream output(logPath, std::ios::app);
    if (!output)
    {
        return {false, "Could not open log file for appending."};
    }

    std::string oneLineEvent = event;
    for (char& character : oneLineEvent)
    {
        if (character == '\n' || character == '\r')
        {
            character = ' ';
        }
    }

    output << timestamp() << " [" << levelName(level) << "] "
           << oneLineEvent << '\n';
    output.close();
    if (!output)
    {
        return {false, "Could not write log entry."};
    }

    return {true, {}};
}
