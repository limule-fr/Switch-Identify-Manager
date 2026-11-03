#include "system_inspector.hpp"

#include <iostream>
#include <string>

#ifdef __SWITCH__
#include <cstring>
#include <switch.h>
#endif

SystemInspectionReport SystemInspector::run() const
{
#ifdef __SWITCH__
    SystemInspectionReport report;
    report.mode = "NINTENDO SWITCH";
    report.inspectionAvailable = true;
    report.integrityCheckPerformed = false;

    Result result = setsysInitialize();
    if (R_FAILED(result))
    {
        report.inspectionAvailable = false;
        report.items.push_back({
            "Version du systeme",
            "Service systeme indisponible : " + std::to_string(result)
        });
    }
    else
    {
        SetSysFirmwareVersion firmware{};
        result = setsysGetFirmwareVersion(&firmware);
        if (R_SUCCEEDED(result))
        {
            report.items.push_back({
                "Version du systeme",
                firmware.display_version
            });
        }
        else
        {
            report.inspectionAvailable = false;
            report.items.push_back({
                "Version du systeme",
                "Lecture impossible : " + std::to_string(result)
            });
        }
        setsysExit();
    }

    result = setInitialize();
    if (R_FAILED(result))
    {
        report.inspectionAvailable = false;
        report.items.push_back({
            "Langue de la console",
            "Service systeme indisponible : " + std::to_string(result)
        });
    }
    else
    {
        u64 languageCode = 0;
        result = setGetSystemLanguage(&languageCode);
        if (R_SUCCEEDED(result))
        {
            char code[sizeof(languageCode) + 1] = {};
            std::memcpy(code, &languageCode, sizeof(languageCode));
            report.items.push_back({
                "Langue de la console",
                std::string(code, strnlen(code, sizeof(code)))
            });
        }
        else
        {
            report.inspectionAvailable = false;
            report.items.push_back({
                "Langue de la console",
                "Lecture impossible : " + std::to_string(result)
            });
        }
        setExit();
    }

    return report;
#else
    return {
        "SIMULATEUR PC",
        false,
        false,
        {
            {
                "Console cible",
                "Nintendo Switch (non inspectee depuis le PC)"
            },
            {
                "Diagnostic materiel",
                "Non disponible dans le simulateur PC"
            },
            {
                "Acces NAND",
                "Non effectue"
            },
            {
                "Acces PRODINFO",
                "Non effectue"
            }
        }
    };
#endif
}

void SystemInspector::displayReport(const Localization& localization) const
{
    const SystemInspectionReport report = run();

    std::cout << "\n";
    std::cout << "========================================\n";
    std::cout << "           " << localization.text("diagnostic.title") << "\n";
    std::cout << "========================================\n";
    std::cout << localization.text("diagnostic.mode") << " : "
              << localization.translate(report.mode) << "\n\n";

    for (const SystemInfoItem& item : report.items)
    {
        std::cout << localization.translate(item.name) << " : "
                  << localization.translate(item.value) << "\n";
    }

    std::cout << "========================================\n";
}
