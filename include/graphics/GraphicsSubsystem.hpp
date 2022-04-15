#pragma once

#include <graphics/DeviceContext.hpp>
#include <graphics/RenderContext.hpp>
#include <graphics/framegraph/FramegraphContext.hpp>

#include <windowing/WindowInterface.hpp>

namespace vanadium::graphics {
	class GraphicsSubsystem {
	  public:
		GraphicsSubsystem(const std::string_view& appName, const std::string_view& pipelineLibraryFileName,
						  uint32_t appVersion, windowing::WindowInterface& interface);
		~GraphicsSubsystem();

		FramegraphContext& framegraphContext() { return m_framegraphContext; }
		const RenderContext& context() { return m_context; }

		void setupFramegraphResources();
		// returns if rendering is possible, if false, waitEvents should be called
		bool tickFrame();

		uint32_t frameIndex() { return m_frameIndex; }

	  private:
		uint32_t m_frameIndex = 0;

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
