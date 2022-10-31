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
#include <filesystem>
#include <vector>
#include <fstream>

#include <Log.hpp>

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

namespace vanadium::graphics {

	constexpr uint32_t assetLibraryVersion = 1;

	struct LibraryImage {
		VkFormat format;
		uint32_t width;
		uint32_t height;
		uint32_t mipCount;
		void* dataStart;
	};

	struct LibraryMesh {
		void* data;
		size_t dataSize;
	};

	struct BinaryHeaderInfo {
		size_t imageCount;
		size_t meshCount;

		size_t binaryDataSize;

		void* meshBinaryDataStart;
        void* imageBinaryDataStart;
        
        std::ifstream dataStream;
	};
    

	class AssetLibrary {
	  public:
		AssetLibrary(const std::string& libraryFile);

		LibraryImage image(uint64_t id) const;
		LibraryMesh mesh(uint64_t id) const;
	  private:
		BinaryHeaderInfo parseLibraryFile();
		void addMesh(BinaryHeaderInfo& headerInfo);
		void addImage(BinaryHeaderInfo& headerInfo);

		template <typename T> T readFromFile(std::ifstream& stream, bool expectEOF = false);

		std::string m_libraryFileName;
		void* m_libraryData;

		std::vector<LibraryImage> m_images;
		std::vector<LibraryMesh> m_meshes;
	};

	template <typename T> T AssetLibrary::readFromFile(std::ifstream& stream, bool expectEOF) {
		T value;
		stream.read(reinterpret_cast<char*>(&value), sizeof(T));
		if (!expectEOF)
			assertFatal(stream.good(), SubsystemID::RHI, "AssetLibrary: Invalid asset library file!");
		return value;
	}

} // namespace vanadium::graphics
