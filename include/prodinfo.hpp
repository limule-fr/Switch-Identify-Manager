#ifndef PRODINFO_HPP
#define PRODINFO_HPP

#include "localization.hpp"

#include <string>

enum class ProdInfoStatus
{
    INTACT,
    MISSING,
    CORRUPTED,
    MASKED
};

class ProdInfoSimulator
{
public:
    ProdInfoSimulator();

    ProdInfoStatus getStatus() const;
    void setStatus(ProdInfoStatus status);

    std::string getStatusName() const;
    void inspect(const Localization& localization = Localization()) const;

private:
    ProdInfoStatus status;
};

#endif