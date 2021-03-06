#include <graphics/pipelines/PipelineLibrary.hpp>
#include <graphics/util/GPUDescriptorSetAllocator.hpp>

namespace vanadium::graphics
{
    inline std::vector<DescriptorSetAllocationInfo> allocationInfosForPipeline(PipelineLibrary* library, PipelineType type, uint32_t pipelineID) {
        std::vector<DescriptorSetLayoutInfo> layoutInfos;
        switch (type)
        {
        case PipelineType::Graphics:
            layoutInfos = library->graphicsPipelineSets(pipelineID);
            break;
        case PipelineType::Compute:
            layoutInfos = library->computePipelineSets(pipelineID);
            break;
        }
        std::vector<DescriptorSetAllocationInfo> result;
        result.reserve(layoutInfos.size());
        for(auto& info : layoutInfos) {
            std::vector<DescriptorTypeInfo> typeInfos;
            typeInfos.reserve(info.bindingInfos.size());
            for(auto& binding : info.bindingInfos) {
                typeInfos.push_back({
                    .type = binding.descriptorType,
                    .count = binding.descriptorCount
                });
            }
            result.push_back({
                .typeInfos = std::move(typeInfos),
                .layout = info.layout
            });
        }
        return result;
    }
} // namespace vanadium::graphics

