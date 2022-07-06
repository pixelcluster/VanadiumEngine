#include <util/SplitString.hpp>
#include <util/WholeFileReader.hpp>
#include <locale>
#include <ui/FontLibrary.hpp>
#include <ui/SystemFontPaths.hpp>

namespace vanadium::ui {
	FontLibrary::FontLibrary(const std::string_view& libraryFileName) {
		FT_Init_FreeType(&m_library);

		size_t bigSize;
		void* data = readFile(libraryFileName.data(), &bigSize);
		if(!data) {
			logWarning("FontLibrary: Couldn't open font library file, no fonts will be used!");
			return;
		}

		uint32_t size = bigSize;
		uint32_t offset = 0;

		std::string fallbackFileName = readString(data, size, offset);
		uint32_t searchPathCount = readCount(data, size, offset);

		m_fontSearchPaths.reserve(searchPathCount + systemFontPathCount);

		for (uint32_t i = 0; i < searchPathCount; ++i) {
			m_fontSearchPaths.push_back(readString(data, size, offset));
		}
		for (const auto& path : systemFontPaths) {
			m_fontSearchPaths.push_back(std::filesystem::path(path));
		}

		uint32_t fontCount = readCount(data, size, offset);
		m_fonts.reserve(fontCount);
		for (uint32_t i = 0; i < fontCount; ++i) {
			uint32_t nameCount = readCount(data, size, offset);
			LibraryFontData font;
			font.names.reserve(nameCount);
			for (uint32_t j = 0; j < nameCount; ++j) {
				font.names.push_back(readString(data, size, offset));
			}
			m_fonts.push_back(font);
		}

		if (!fallbackFileName.empty()) {
			FT_New_Face(m_library, fallbackFileName.c_str(), 0, &m_fallbackFace);
		}

		for (auto& font : m_fonts) {
			for (auto& name : font.names) {
				if (font.fontFace != nullptr)
					break;

				for (auto& path : m_fontSearchPaths) {
					FT_Face face = tryCreateFace(path, name);
					if (face) {
						font.fontFace = face;
						break;
					}
				}
			}
			if (!font.fontFace && m_fallbackFace) {
				if (!font.names.empty())
					logWarning("FontLibrary: Font {} not found, using fallback.", font.names[0].c_str());
				font.fontFace = m_fallbackFace;
			}
		}

		delete[] reinterpret_cast<char*>(data);
	}

	FT_Face FontLibrary::tryCreateFace(const std::filesystem::path& searchPath, const std::string& name) {
		for (auto& entry : std::filesystem::directory_iterator(searchPath)) {
			if (entry.is_directory()) {
				FT_Face face = tryCreateFace(entry.path(), name);
				if (face)
					return face;
			} else if (entry.is_regular_file()) {
				FT_Long faceCount = 0;
				FT_Face dummyFace = nullptr;

				std::string entryPathString = entry.path().string();
				FT_Error error = FT_New_Face(m_library, entryPathString.c_str(), -1, &dummyFace);
				if (error == FT_Err_Unknown_File_Format) {
					continue;
				} else if (error != FT_Err_Ok) {
					logError("FontLibrary: Error opening face {}!", entryPathString.c_str());
				} else {
					faceCount = dummyFace->num_faces;
					FT_Done_Face(dummyFace);
				}
				for (FT_Long i = 0; i < faceCount; ++i) {
					FT_Face face;
					FT_Error error = FT_New_Face(m_library, entryPathString.c_str(), i, &face);
					if (error != FT_Err_Ok) {
						logError("FontLibrary: Error opening face {}!", entryPathString.c_str());
					}
					if (name == std::string(face->family_name) ||
						name == std::string(face->family_name) + " " + std::string(face->style_name)) {
						logInfo("FontLibrary: Found name {} in path {}", name.c_str(), entryPathString.c_str());
						return face;
					}
					FT_Done_Face(face);
				}
			}
		}
		return nullptr;
	}

	FontLibrary::~FontLibrary() { FT_Done_FreeType(m_library); }
} // namespace vanadium::ui
