#include <graphics/GraphicsSubsystem.hpp>
#include <volk.h>

namespace vanadium::graphics {
	GraphicsSubsystem::GraphicsSubsystem(const std::string_view& appName, uint32_t appVersion,
										 windowing::WindowInterface& interface, bool initFramegraph)
		: m_hasFramegraph(initFramegraph), m_surface(interface), m_deviceContext(appName, appVersion, m_surface),
		  m_renderTargetSurface(&m_deviceContext) {
		m_resourceAllocator.create(&m_deviceContext);
		m_descriptorSetAllocator.create(&m_deviceContext);
		m_transferManager.create(&m_deviceContext, &m_resourceAllocator);

		if (initFramegraph) {
			m_renderTargetSurface.create(m_surface.swapchainImages(m_deviceContext.device()),
										 { .width = m_surface.imageWidth(),
										   .height = m_surface.imageHeight(),
										   .format = m_surface.swapchainImageFormat() });

			m_framegraphContext.create(&m_deviceContext, &m_resourceAllocator, &m_descriptorSetAllocator,
									   &m_transferManager, &m_renderTargetSurface);
		}
	}

	bool GraphicsSubsystem::tickFrame() {
		++frameIndex %= frameInFlightCount;
		vkWaitForFences(m_deviceContext.device(), 1, &m_deviceContext.frameCompletionFence(frameIndex), VK_TRUE,
						UINT64_MAX);
		vkResetFences(m_deviceContext.device(), 1, &m_deviceContext.frameCompletionFence(frameIndex));

		if (m_surface.swapchainDirtyFlag()) {
			m_surface.createSwapchain(m_deviceContext.physicalDevice(), m_deviceContext.device(),
									  m_framegraphContext.targetImageUsageFlags());
		}

		uint32_t imageIndex = m_surface.tryAcquire(m_deviceContext.device(), frameIndex);
		if (m_surface.canRender()) {
			if (m_hasFramegraph) {
				m_renderTargetSurface.setTargetImageIndex(imageIndex);

				VkCommandBuffer commandBuffers[2]{ m_transferManager.recordTransfers(frameIndex),
												   m_framegraphContext.recordFrame(frameIndex) };

				VkSubmitInfo submitInfo = { .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
											.waitSemaphoreCount = 1,
											.pWaitSemaphores = &m_surface.acquireSemaphore(frameIndex),
											.commandBufferCount = 2,
											.pCommandBuffers = commandBuffers,
											.signalSemaphoreCount = 1,
											.pSignalSemaphores = &m_surface.presentSemaphore(frameIndex) };
				vkQueueSubmit(m_deviceContext.graphicsQueue(), 1, &submitInfo,
							  m_deviceContext.frameCompletionFence(frameIndex));
			}

			m_surface.tryPresent(m_deviceContext.graphicsQueue(), imageIndex, frameIndex);
			return true;
		} else {
			return false;
		}
	}
} // namespace vanadium::graphics
