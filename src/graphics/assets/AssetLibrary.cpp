#include <graphics/assets/AssetLibrary.hpp>
#include <fstream>
#include <iostream>
#include <Log.hpp>

namespace vanadium::graphics
{
    AssetLibrary::AssetLibrary(const std::string& libraryFile)
    {

    }

    BinaryHeaderInfo AssetLibrary::parseLibraryFile() {
        std::ifstream inFileStream = std::ifstream(m_libraryFileName);
        assertFatal(inFileStream.is_open(), "Invalid asset library file!\n");

        uint32_t version;
        inFileStream.read(reinterpret_cast<char*>(&version), sizeof(uint32_t));

        assertFatal(version == assetLibraryVersion, "Invalid asset library version!\n");

        //TODO: read sizes and counts of images/meshes/data
    }
} // namespace vanadium::graphics
