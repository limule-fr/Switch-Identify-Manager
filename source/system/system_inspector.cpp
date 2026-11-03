#include "system_inspector.hpp"

#include <iostream>

SystemInspectionReport SystemInspector::run() const
{
    return {
        "SIMULATEUR PC",
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
