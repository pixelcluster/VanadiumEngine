#pragma once
#include <filesystem>
#include <util/Vector.hpp>
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

		SimpleVector<LibraryImage> m_images;
		SimpleVector<LibraryMesh> m_meshes;
	};

	template <typename T> T AssetLibrary::readFromFile(std::ifstream& stream, bool expectEOF) {
		T value;
		stream.read(reinterpret_cast<char*>(&value), sizeof(T));
		if (!expectEOF)
			assertFatal(stream.good(), "AssetLibrary: Invalid asset library file!");
		return value;
	}

} // namespace vanadium::graphics