#pragma once

#include <array>
#include <string_view>

namespace vanadium::ui
{
    #if defined(__linux__)
    constexpr uint32_t systemFontPathCount = 3;
    constexpr std::array<std::string_view, systemFontPathCount> systemFontPaths = {
        "/usr/share/fonts",
        "/usr/local/share/fonts",
        "~/.fonts"
    };
    #elif defined(_WIN32)
    constexpr uint32_t systemFontPathCount = 1;
    constexpr std::array<std::string_view, systemFontPathCount> systemFontPaths = {
        "C:\\Windows\\Fonts"
    };
    #else  
    #error Unsupported Platform!
    #endif
} // namespace vanadium::ui
