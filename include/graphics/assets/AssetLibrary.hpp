#include <filesystem>
#include <vector>

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

namespace vanadium::graphics {
    struct LibraryBuffer
    {
        uint64_t id;
        void* data;
        size_t dataSize;
    };

    struct LibraryImageLOD
    {
        uint64_t id;
        uint32_t width;
        uint32_t height;
        VkFormat format;
    };
    
    struct LibraryImage
    {
        uint64_t id;
        std::vector<LibraryImageLOD> lods;
    };

    struct LibraryMesh
    {
        uint64_t id;
        std::vector<LibraryBuffer> lods;
    };

    class AssetLibrary
    {
    public:
        AssetLibrary(const std::string& libraryFile);
    private:
        std::string m_libraryFileName;
        void* libraryData;

    };
    
}