#pragma once

#include <Log.hpp>
#include <cstring>
#include <ft2build.h>
#include <hb-ft.h>
#include <hb.h>
#include <string_view>

namespace vanadium::ui {

    struct LibraryFontData
    {
        std::vector<std::string> names;
        FT_Face fontFace;
    };
    

	class FontLibrary {
	  public:
		FontLibrary(const std::string_view& registryFileName);
		~FontLibrary();

	  private:
		uint32_t readCount(void* data, uint32_t size, uint32_t& offset) {
			assertFatal(offset + sizeof(uint32_t) <= size, "FontLibrary: Out-of-bounds read!\n");
			uint32_t value;
			std::memcpy(&value, reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(data) + offset), sizeof(uint32_t));
            offset += sizeof(uint32_t);
            return value;
		}
        std::string readString(void* data, uint32_t size, uint32_t& offset) {
            uint32_t stringOffset = readCount(data, size, offset);
            uint32_t stringSize = readCount(data, size, offset);
			assertFatal(stringOffset + stringSize <= size, "FontLibrary: Out-of-bounds read!\n");
            std::string string = std::string(stringSize, ' ');
            std::memcpy(string.data(), reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(data) + stringOffset), stringSize * sizeof(char));
            return string;
        }

        std::vector<std::string> m_fontSearchPaths;
        std::vector<LibraryFontData> m_fonts;

		FT_Library m_library;
	};

} // namespace vanadium::ui
