#include <ui/FontLibrary.hpp>
#include <helper/WholeFileReader.hpp>

namespace vanadium::ui
{
    FontLibrary::FontLibrary(const std::string_view& libraryFileName) {
        size_t bigSize;
        void* data = readFile(libraryFileName.data(), &bigSize);
        assertFatal(data, "FontLibrary: Couldn't open font library file!\n");

        uint32_t size = bigSize;
        uint32_t offset = 0;

        std::string fallbackFileName = readString(data, size, offset);
        uint32_t searchPathCount = readCount(data, size, offset);
        m_fontSearchPaths.reserve(searchPathCount);
        for(uint32_t i = 0; i < searchPathCount; ++i) {
            m_fontSearchPaths.push_back(readString(data, size, offset));
        }

        uint32_t fontCount = readCount(data, size, offset);
        m_fonts.reserve(fontCount);
        for(uint32_t i = 0; i < fontCount; ++i) {
            uint32_t nameCount = readCount(data, size, offset);
            LibraryFontData font;
            font.names.reserve(nameCount);
            for(uint32_t j = 0; j < nameCount; ++j) {
                font.names.push_back(readString(data, size, offset));
            }
            m_fonts.push_back(font);
        }
    }

    FontLibrary::~FontLibrary() {

    }
} // namespace vanadium::ui
