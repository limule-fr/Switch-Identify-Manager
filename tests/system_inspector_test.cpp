#include "system_inspector.hpp"

#include <iostream>

int main()
{
    const SystemInspectionReport report = SystemInspector().run();
    if (report.mode != "SIMULATEUR PC")
    {
        std::cerr << "System inspector did not identify its PC mode.\n";
        return 1;
    }
    if (report.inspectionAvailable)
    {
        std::cerr << "PC simulator unexpectedly reports hardware inspection.\n";
        return 1;
    }

    bool hasNandAccessNotice = false;
    bool hasProdInfoAccessNotice = false;
    bool hasUnavailableDiagnosticNotice = false;
    for (const SystemInfoItem& item : report.items)
    {
        if (item.name == "Acces NAND" && item.value == "Non effectue")
        {
            hasNandAccessNotice = true;
        }
        if (item.name == "Acces PRODINFO" && item.value == "Non effectue")
        {
            hasProdInfoAccessNotice = true;
        }
        if (item.name == "Diagnostic materiel"
            && item.value == "Non disponible dans le simulateur PC")
        {
            hasUnavailableDiagnosticNotice = true;
        }
    }

    if (!hasNandAccessNotice
        || !hasProdInfoAccessNotice
        || !hasUnavailableDiagnosticNotice)
    {
        std::cerr << "System inspector safety/status report is incomplete.\n";
        return 1;
    }

    std::cout << "All system inspector tests passed.\n";
    return 0;
}
