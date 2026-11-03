#include "integrity_checker.hpp"

#include <iostream>
#include <string>

namespace
{
    bool checkStatus(
        ProdInfoStatus prodInfoStatus,
        int expectedScore,
        const std::string& expectedGeneralStatus,
        CheckStatus expectedCheckStatus
    )
    {
        ProdInfoSimulator prodInfo;
        prodInfo.setStatus(prodInfoStatus);

        const IntegrityReport report = IntegrityChecker(prodInfo).run();
        bool passed = true;

        if (report.score != expectedScore)
        {
            std::cerr << "Expected score " << expectedScore
                      << ", got " << report.score << ".\n";
            passed = false;
        }

        if (report.generalStatus != expectedGeneralStatus)
        {
            std::cerr << "Expected general status " << expectedGeneralStatus
                      << ", got " << report.generalStatus << ".\n";
            passed = false;
        }

        if (report.items.size() != 4)
        {
            std::cerr << "Expected 4 integrity items, got "
                      << report.items.size() << ".\n";
            return false;
        }

        if (report.items.front().status != expectedCheckStatus)
        {
            std::cerr << "Unexpected PRODINFO check status.\n";
            passed = false;
        }

        int totalMaximumPoints = 0;
        for (const IntegrityItem& item : report.items)
        {
            totalMaximumPoints += item.maxPoints;
            if (item.points < 0 || item.points > item.maxPoints)
            {
                std::cerr << "Invalid points for " << item.name << ".\n";
                passed = false;
            }
        }

        if (totalMaximumPoints != 100)
        {
            std::cerr << "Expected a maximum score of 100, got "
                      << totalMaximumPoints << ".\n";
            passed = false;
        }

        return passed;
    }
}

int main()
{
    bool passed = true;
    passed &= checkStatus(
        ProdInfoStatus::INTACT, 100, "EXCELLENT", CheckStatus::OK
    );
    passed &= checkStatus(
        ProdInfoStatus::MASKED, 85, "BON", CheckStatus::WARNING
    );
    passed &= checkStatus(
        ProdInfoStatus::MISSING, 60, "DEGRADE", CheckStatus::ERROR
    );
    passed &= checkStatus(
        ProdInfoStatus::CORRUPTED, 60, "DEGRADE", CheckStatus::ERROR
    );

    if (!passed)
    {
        return 1;
    }

    std::cout << "All integrity checker tests passed.\n";
    return 0;
}
