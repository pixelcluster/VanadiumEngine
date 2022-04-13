#pragma once

#include <graphics/DeviceContext.hpp>
#include <graphics/RenderContext.hpp>
#include <graphics/framegraph/FramegraphContext.hpp>

#include <windowing/WindowInterface.hpp>

namespace vanadium::graphics {
	class GraphicsSubsystem {
	  public:
		GraphicsSubsystem(const std::string_view& appName, const std::string_view& pipelineLibraryFileName,
						  uint32_t appVersion, windowing::WindowInterface& interface, bool initFramegraph);
		 ~GraphicsSubsystem();

		 

		// returns if rendering is possible, if false, waitEvents should be called
		bool tickFrame();


	  private:
		bool m_hasFramegraph;
		uint32_t frameIndex = 0;

		WindowSurface m_surface;
		DeviceContext m_deviceContext;
		GPUResourceAllocator m_resourceAllocator;
		GPUDescriptorSetAllocator m_descriptorSetAllocator;
		GPUTransferManager m_transferManager;
		RenderTargetSurface m_renderTargetSurface;
		PipelineLibrary m_pipelineLibrary;
		FramegraphContext m_framegraphContext;

		RenderContext m_context;
	};

} // namespace vanadium::graphics
