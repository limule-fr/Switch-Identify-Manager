#include "prodinfo.hpp"

#include <iostream>

ProdInfoSimulator::ProdInfoSimulator()
    : status(ProdInfoStatus::INTACT)
{
}

ProdInfoStatus ProdInfoSimulator::getStatus() const
{
    return status;
}

void ProdInfoSimulator::setStatus(ProdInfoStatus newStatus)
{
    status = newStatus;
}

std::string ProdInfoSimulator::getStatusName() const
{
    switch (status)
    {
        case ProdInfoStatus::INTACT:
            return "INTACT";

        case ProdInfoStatus::MISSING:
            return "MISSING";

        case ProdInfoStatus::CORRUPTED:
            return "CORRUPTED";

        case ProdInfoStatus::MASKED:
            return "MASKED";
    }

    return "UNKNOWN";
}

void ProdInfoSimulator::inspect(const Localization& localization) const
{
    std::cout << "\n========== " << localization.text("prodinfo.title")
              << " ==========\n";

    std::cout << localization.text("prodinfo.mode") << "\n";
    std::cout << localization.text("label.status") << " : "
              << getStatusName() << "\n\n";

    switch (status)
    {
        case ProdInfoStatus::INTACT:
            std::cout << localization.text("prodinfo.intact_present") << "\n";
            std::cout << localization.text("prodinfo.intact_consistent") << "\n";
            break;

        case ProdInfoStatus::MISSING:
            std::cout << localization.text("prodinfo.missing") << "\n";
            std::cout << localization.text("prodinfo.no_data") << "\n";
            break;

        case ProdInfoStatus::CORRUPTED:
            std::cout << localization.text("prodinfo.corrupted") << "\n";
            std::cout << localization.text("prodinfo.integrity_compromised")
                      << "\n";
            break;

        case ProdInfoStatus::MASKED:
            std::cout << localization.text("prodinfo.masked") << "\n";
            std::cout << localization.text("prodinfo.masked_content") << "\n";
            break;
    }

    std::cout << "=========================================\n";
}