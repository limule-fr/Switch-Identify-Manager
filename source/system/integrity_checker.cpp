#include "integrity_checker.hpp"

#include <iostream>

IntegrityChecker::IntegrityChecker(
    const ProdInfoSimulator& prodInfo
)
    : prodInfo(prodInfo)
{
}

IntegrityReport IntegrityChecker::run() const
{
    IntegrityReport report;

    report.score = 0;

    // 1. Inspection PRODINFO : 40 points
    switch (prodInfo.getStatus())
    {
        case ProdInfoStatus::INTACT:
            report.items.push_back({
                "PRODINFO",
                CheckStatus::OK,
                40,
                40,
                "Donnees simulees coherentes."
            });
            break;

        case ProdInfoStatus::MASKED:
            report.items.push_back({
                "PRODINFO",
                CheckStatus::WARNING,
                25,
                40,
                "Donnees simulees masquees."
            });
            break;

        case ProdInfoStatus::MISSING:
            report.items.push_back({
                "PRODINFO",
                CheckStatus::ERROR,
                0,
                40,
                "Donnees simulees absentes."
            });
            break;

        case ProdInfoStatus::CORRUPTED:
            report.items.push_back({
                "PRODINFO",
                CheckStatus::ERROR,
                0,
                40,
                "Anomalie simulee detectee."
            });
            break;
    }

    // 2. Donnees systeme : 25 points
    report.items.push_back({
        "SYSTEM DATA",
        CheckStatus::OK,
        25,
        25,
        "Structure fictive coherente."
    });

    // 3. Sauvegarde : 20 points
    report.items.push_back({
        "BACKUP",
        CheckStatus::OK,
        20,
        20,
        "Sauvegarde fictive disponible."
    });

    // 4. Configuration : 15 points
    report.items.push_back({
        "CONFIGURATION",
        CheckStatus::OK,
        15,
        15,
        "Configuration fictive valide."
    });

    // Calcul du score
    for (const IntegrityItem& item : report.items)
    {
        report.score += item.points;
    }

    // Etat general
    if (report.score == 100)
    {
        report.generalStatus = "EXCELLENT";
    }
    else if (report.score >= 75)
    {
        report.generalStatus = "BON";
    }
    else if (report.score >= 50)
    {
        report.generalStatus = "DEGRADE";
    }
    else
    {
        report.generalStatus = "CRITIQUE";
    }

    return report;
}

std::string IntegrityChecker::statusToString(
    CheckStatus status
) const
{
    switch (status)
    {
        case CheckStatus::OK:
            return "OK";

        case CheckStatus::WARNING:
            return "WARNING";

        case CheckStatus::ERROR:
            return "ERROR";
    }

    return "UNKNOWN";
}

void IntegrityChecker::displayReport(const Localization& localization) const
{
    IntegrityReport report = run();

    std::cout << "\n";
    std::cout << "========================================\n";
    std::cout << "           " << localization.text("integrity.title") << "\n";
    std::cout << "========================================\n\n";

    std::cout << localization.text("integrity.mode") << "\n";
    std::cout << localization.text("integrity.disclaimer") << "\n\n";

    for (const IntegrityItem& item : report.items)
    {
        std::cout << localization.translate(item.name)
                  << " : "
                  << localization.translate(statusToString(item.status))
                  << "\n";

        std::cout << "  " << localization.translate(item.message) << "\n";
        std::cout << "  " << localization.text("integrity.points") << " : "
                  << item.points << "/"
                  << item.maxPoints << "\n\n";
    }

    std::cout << "----------------------------------------\n";

    std::cout << localization.text("integrity.score") << " : "
              << report.score << "/100\n";

    std::cout << localization.text("integrity.general_status") << " : "
              << localization.translate(report.generalStatus) << "\n";

    std::cout << "========================================\n";
}