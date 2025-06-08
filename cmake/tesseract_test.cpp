#include <iostream>
#include <string>
#include <tesseract/baseapi.h>
#include <vector>

int main()
{
    tesseract::TessBaseAPI api;

    if (api.Init(nullptr, nullptr) != 0) {
        std::cerr << "Failed to initialize Tesseract" << std::endl;
        return 1;
    }

    std::vector<std::string> languages;
    api.GetAvailableLanguagesAsVector(&languages);

    // Filter out 'osd' as it's not a usable language for OCR
    std::vector<std::string> usableLanguages;
    for (const auto &lang : languages) {
        if (lang != "osd") {
            usableLanguages.push_back(lang);
        }
    }

    if (usableLanguages.empty()) {
        std::cerr << "No usable Tesseract language packs found. Install language data files (e.g., tesseract-ocr-eng)" << std::endl;
        return 1;
    }

    std::cout << "Found " << usableLanguages.size() << " Tesseract language pack(s): ";
    for (size_t i = 0; i < usableLanguages.size(); ++i) {
        std::cout << usableLanguages[i];
        if (i < usableLanguages.size() - 1)
            std::cout << ", ";
    }
    std::cout << std::endl;

    return 0;
}
