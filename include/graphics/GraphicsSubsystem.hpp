#pragma once

#include <graphics/DeviceContext.hpp>
#include <graphics/framegraph/FramegraphContext.hpp>
#include <graphics/framegraph/RenderTargetSurface.hpp>

#include <windowing/WindowInterface.hpp>

namespace vanadium::graphics
{
    class GraphicsSubsystem
    {
    public:
        GraphicsSubsystem(const std::string_view& appName, uint32_t appVersion, windowing::WindowInterface& interface, bool initFramegraph);

        ~GraphicsSubsystem();
    private:
        bool m_hasFramegraph;

        WindowSurface m_surface;
        DeviceContext m_deviceContext;
        GPUResourceAllocator m_resourceAllocator;
        GPUDescriptorSetAllocator m_descriptorSetAllocator;
        GPUTransferManager m_transferManager;
        RenderTargetSurface m_renderTargetSurface;
        FramegraphContext m_framegraphContext;
    };
    
} // namespace vanadium::graphics
