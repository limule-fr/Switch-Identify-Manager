#ifndef SYSTEM_INSPECTOR_HPP
#define SYSTEM_INSPECTOR_HPP

#include "localization.hpp"

#include <string>
#include <vector>

struct SystemInfoItem
{
    std::string name;
    std::string value;
};

struct SystemInspectionReport
{
    std::string mode;
    std::vector<SystemInfoItem> items;
};

class SystemInspector
{
public:
    SystemInspectionReport run() const;
    void displayReport(
        const Localization& localization = Localization()
    ) const;
};

#endif
