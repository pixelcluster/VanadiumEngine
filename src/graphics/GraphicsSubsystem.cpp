#include <graphics/GraphicsSubsystem.hpp>

namespace vanadium::graphics {
	GraphicsSubsystem::GraphicsSubsystem(const std::string_view& appName, uint32_t appVersion,
										 windowing::WindowInterface& interface, bool initFramegraph)
		: m_hasFramegraph(initFramegraph), m_surface(interface), m_deviceContext(appName, appVersion, m_surface),
		  m_renderTargetSurface(&m_deviceContext) {
		m_resourceAllocator.create(&m_deviceContext);
		m_descriptorSetAllocator.create(&m_deviceContext);
		m_transferManager.create(&m_deviceContext, &m_resourceAllocator);

		m_renderTargetSurface.create(m_surface.swapchainImages(m_deviceContext.device()),
									 { .width = m_surface.imageWidth(),
									   .height = m_surface.imageHeight(),
									   .format = m_surface.swapchainImageFormat() });

		m_framegraphContext.create(&m_deviceContext, &m_resourceAllocator, &m_descriptorSetAllocator,
								   &m_transferManager, &m_renderTargetSurface);
	}
} // namespace vanadium::graphics
