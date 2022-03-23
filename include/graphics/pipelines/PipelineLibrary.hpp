#pragma once

#include <vector>
#include <variant>
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

namespace vanadium::graphics {

    #include <tools/vcp/include/PipelineStructs.hpp>

    struct PipelineLibraryArchetype
    {
        PipelineType type;
        std::vector<VkShaderModule> m_shaders;
        std::vector<VkDescriptorSetLayoutCreateInfo> m_setLayoutCreateInfos;
        std::vector<VkDescriptorSetLayoutBinding> m_bindingInfos;
        std::vector<VkPushConstantRange> m_pushConstantRanges;
    };

    struct PipelineLibraryInstance
    {
        std::vector<VkVertexInputAttributeDescription> attribDescriptions;
        std::vector<VkVertexInputBindingDescription> attribDescriptions;
        VkPipelineVertexInputStateCreateInfo vertexInputConfig;
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyConfig;
        VkPipelineRasterizationStateCreateInfo rasterizationConfig;
        VkPipelineMultisampleStateCreateInfo multisampleConfig;
        InstanceDepthStencilConfig depthStencilConfig;
        InstanceColorBlendConfig colorBlendConfig;
        std::vector<InstanceColorAttachmentBlendConfig> colorAttachmentBlendConfigs;
        InstanceSpecializationConfig
    };
    
    

    class PipelineLibrary
    {
    public:
        PipelineLibrary();
    private:

    };
    
}