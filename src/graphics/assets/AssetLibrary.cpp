#include <Log.hpp>
#include <fstream>
#include <graphics/assets/AssetLibrary.hpp>
#include <iostream>

void* offsetVoidPtr(void* data, uint64_t size) {
	return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(data) + size);
}

namespace vanadium::graphics {
	AssetLibrary::AssetLibrary(const std::string& libraryFile) : m_libraryFileName(libraryFile) {
		BinaryHeaderInfo headerInfo = parseLibraryFile();
		for (uint32_t i = 0; i < headerInfo.meshCount; ++i) {
			addMesh(headerInfo);
		}
		for (uint32_t i = 0; i < headerInfo.imageCount; ++i) {
			addImage(headerInfo);
		}
		headerInfo.dataStream.read(reinterpret_cast<char*>(headerInfo.meshBinaryDataStart), headerInfo.binaryDataSize);
		headerInfo.dataStream.close();
	}

	BinaryHeaderInfo AssetLibrary::parseLibraryFile() {
		std::ifstream inFileStream = std::ifstream(m_libraryFileName);
		assertFatal(inFileStream.is_open(), "AssetLibrary: Invalid asset library file!\n");
		assertFatal(inFileStream.good(), "AssetLibrary: Invalid asset library file!\n");

		uint32_t version = readFromFile<uint32_t>(inFileStream);
		assertFatal(version == assetLibraryVersion, "AssetLibrary: Invalid asset library version!\n");

		uint32_t meshCount = readFromFile<uint32_t>(inFileStream);
		uint32_t imageCount = readFromFile<uint32_t>(inFileStream);

		uint32_t imageDataSize = readFromFile<uint32_t>(inFileStream);

		uint64_t meshBinaryDataSize = readFromFile<uint32_t>(inFileStream);
		uint64_t imageBinaryDataSize = readFromFile<uint32_t>(inFileStream);

		uint64_t totalDataSize = meshBinaryDataSize + imageBinaryDataSize;
		void* data = new char[totalDataSize];
		assertFatal(data, "AssetLibrary: Failed to allocate data for asset library!\n");

		return { .imageCount = imageCount,
				 .meshCount = meshCount,
				 .binaryDataSize = totalDataSize,
				 .meshBinaryDataStart = data,
				 .imageBinaryDataStart = offsetVoidPtr(data, meshBinaryDataSize),
				 .dataStream = std::move(inFileStream) };
	}

	void AssetLibrary::addMesh(BinaryHeaderInfo& headerInfo) {
		uint32_t dataSize = readFromFile<uint32_t>(headerInfo.dataStream);
		uint64_t dataOffset = readFromFile<uint64_t>(headerInfo.dataStream);

		m_meshes.push_back({ .data = offsetVoidPtr(headerInfo.meshBinaryDataStart, dataOffset), .dataSize = dataSize });
	}

	void AssetLibrary::addImage(BinaryHeaderInfo& headerInfo) {
		uint32_t binaryFormat = readFromFile<uint32_t>(headerInfo.dataStream);
		uint32_t imageWidth = readFromFile<uint32_t>(headerInfo.dataStream);
		uint32_t imageHeight = readFromFile<uint32_t>(headerInfo.dataStream);
		uint64_t dataOffset = readFromFile<uint64_t>(headerInfo.dataStream);
		uint32_t mipCount = readFromFile<uint32_t>(headerInfo.dataStream);

		m_images.push_back({ .format = static_cast<VkFormat>(binaryFormat),
							 .width = imageWidth,
							 .height = imageHeight,
							 .mipCount = mipCount,
							 .dataStart = offsetVoidPtr(headerInfo.imageBinaryDataStart, dataOffset) });
	}

	LibraryImage AssetLibrary::image(uint64_t id) const {
		if (id < m_images.size())
			return m_images[id];
		else
			return {};
	}

	LibraryMesh AssetLibrary::mesh(uint64_t id) const {
		if (id < m_meshes.size())
			return m_meshes[id];
		else
			return {};
	}
} // namespace vanadium::graphics
