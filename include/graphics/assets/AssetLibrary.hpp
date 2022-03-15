#include <filesystem>
#include <vector>

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

namespace vanadium::graphics {

    constexpr uint32_t assetLibraryVersion = 1;

    struct LibraryImageMipmap
    {
        uint32_t width;
        uint32_t height;
        void* data;
    };
    
    struct LibraryImage
    {
        VkFormat format;
        std::vector<uint64_t> mipmapIDs;
    };

    struct LibraryMesh
    {
        void* data;
        size_t dataSize;
    };

    struct BinaryHeaderInfo
    {
        size_t imageCount;
        size_t meshCount;

        void* dataStart;
        void* meshStart;
        void* imageStart;
        void* dataStart;
    };
    

    class AssetLibrary
    {
    public:
        AssetLibrary(const std::string& libraryFile);
    private:
        BinaryHeaderInfo parseLibraryFile();

        std::string m_libraryFileName;
        void* m_libraryData;

        std::vector<LibraryImage> m_images;
        std::vector<LibraryMesh> m_meshes;
    };
    
}