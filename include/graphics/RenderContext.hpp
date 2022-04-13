#pragma once

#include <graphics/DeviceContext.hpp>
#include <graphics/RenderTargetSurface.hpp>
#include <graphics/util/GPUDescriptorSetAllocator.hpp>
#include <graphics/util/GPUResourceAllocator.hpp>
#include <graphics/util/GPUTransferManager.hpp>
#include <graphics/pipelines/PipelineLibrary.hpp>

namespace vanadium::graphics
{
    struct RenderContext
    {
        DeviceContext* deviceContext;
		GPUResourceAllocator* resourceAllocator;
		GPUDescriptorSetAllocator* descriptorSetAllocator;
		GPUTransferManager* transferManager;
		PipelineLibrary* pipelineLibrary;
		RenderTargetSurface* targetSurface;
    };
    
} // namespace vanadium::graphics
