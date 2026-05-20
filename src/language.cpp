#include "language.h"

#include <cstdio>
#include <filesystem>

#include <windows.h>

#include <easy_translate.hpp>

#include <config.h>

const char* getLanguageStringId(Language lang)
{
    switch (lang)
    {
        case LANG_EN:   return "EN";
        case LANG_ZH:   return "ZH";
        default:        return "";
    }
}

Language getCurrentSystemLang()
{
    LANGID langId = GetUserDefaultUILanguage();
    if (PRIMARYLANGID(langId) == LANG_CHINESE)
        return LANG_ZH;
    return LANG_EN;
}

class DirectoryScope
{
public:
    explicit DirectoryScope(const std::string& path)
    {
        originPath_ = std::filesystem::current_path();
        std::filesystem::current_path(path);
    }

    ~DirectoryScope()
    {
        std::filesystem::current_path(originPath_);
    }

private:
    std::filesystem::path originPath_;
};

bool setLanguage(Language lang)
{
    // Ensure CWD == exe directory so relative paths (language files) always resolve,
    // even when launched via the registry auto-start mechanism.
    char exePath[MAX_PATH] = {};
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    std::string exeDir(exePath);
    auto sep = exeDir.find_last_of("\\/");
    if (sep != std::string::npos)
        exeDir = exeDir.substr(0, sep);

    {
        DirectoryScope dirScope(exeDir);
        easytr::setLanguages(easytr::Languages::fromFile(APP_LANG_FILEPATH));
    }

    if (easytr::languages().empty())
    {
        fprintf(stderr, "[Language] Failed to load languages or languages list is empty.");
        return false;
    }

    const char* id = getLanguageStringId(lang);
    if (!easytr::hasLanguage(id))
    {
        fprintf(stderr, "[Language] Language %s is not available.", id);
        return false;
    }

    {
        DirectoryScope dirScope(exeDir);
        if (!easytr::setCurrentLanguage(id))
        {
            fprintf(stderr, "[Language] Failed to set the current language to %s.", id);
            return false;
        }
    }

    // Note: persistent UI elements (e.g. tray tooltip) should be updated by the caller.

    return true;
}
