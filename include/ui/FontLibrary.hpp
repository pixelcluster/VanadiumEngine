/* VanadiumEngine, a Vulkan rendering toolkit
 * Copyright (C) 2022 Friedrich Vock
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#pragma once

#include <Log.hpp>
#include <cstring>
#include <filesystem>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <string_view>
#include <vector>

namespace vanadium::ui {

	struct LibraryFontData {
		std::vector<std::string> names;
		FT_Face fontFace = nullptr;
	};

	class FontLibrary {
	  public:
		FontLibrary(const std::string_view& registryFileName);
		FontLibrary(const FontLibrary&) = delete;
		FontLibrary& operator=(const FontLibrary&) = delete;
		FontLibrary(FontLibrary&&) = delete;
		FontLibrary& operator=(FontLibrary&&) = delete;
		~FontLibrary();

		FT_Face fontFace(uint32_t fontID) { return m_fonts[fontID].fontFace; }

	  private:
		uint32_t readCount(void* data, uint32_t size, uint32_t& offset) {
			assertFatal(offset + sizeof(uint32_t) <= size, SubsystemID::RHI, "FontLibrary: Out-of-bounds read!");
			uint32_t value;
			std::memcpy(&value, reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(data) + offset), sizeof(uint32_t));
			offset += sizeof(uint32_t);
			return value;
		}
		std::string readString(void* data, uint32_t size, uint32_t& offset) {
			uint32_t stringOffset = readCount(data, size, offset);
			uint32_t stringSize = readCount(data, size, offset);
			assertFatal(stringOffset + stringSize <= size, SubsystemID::RHI, "FontLibrary: Out-of-bounds read!");
			std::string string = std::string(stringSize, ' ');
			std::memcpy(string.data(), reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(data) + stringOffset),
						stringSize * sizeof(char));
			return string;
		}

		FT_Face tryCreateFace(const std::filesystem::path& searchPath, const std::string& name);

		std::vector<std::filesystem::path> m_fontSearchPaths;
		std::vector<LibraryFontData> m_fonts;

		FT_Face m_fallbackFace = nullptr;

		FT_Library m_library;
	};

} // namespace vanadium::ui
