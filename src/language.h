#pragma once

#include <string>

enum Language : unsigned char
{
    LANG_EN = 0,
    LANG_ZH
};

const char* getLanguageStringId(Language lang);

// If the application does not support the current system language, return LANG_EN.
Language getCurrentSystemLang();

bool setLanguage(Language lang);
