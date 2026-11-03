#ifndef LOCALIZATION_HPP
#define LOCALIZATION_HPP

#include <string>

enum class Language
{
    FRENCH,
    ENGLISH,
    SPANISH
};

class Localization
{
public:
    explicit Localization(Language language = Language::FRENCH);

    Language language() const;
    std::string languageCode() const;
    std::string languageName() const;
    std::string text(const std::string& key) const;
    std::string translate(const std::string& frenchText) const;
    std::string translateSystemLanguage(const std::string& languageCode) const;

private:
    Language currentLanguage;
};

#endif
