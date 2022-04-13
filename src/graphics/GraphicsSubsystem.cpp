#include <graphics/GraphicsSubsystem.hpp>
#include <volk.h>

namespace vanadium::graphics {
	GraphicsSubsystem::GraphicsSubsystem(const std::string_view& appName,
										 const std::string_view& pipelineLibraryFileName, uint32_t appVersion,
										 windowing::WindowInterface& interface, bool initFramegraph)
		: m_hasFramegraph(initFramegraph), m_surface(interface), m_deviceContext(appName, appVersion, m_surface),
		  m_renderTargetSurface(&m_deviceContext) {
		m_resourceAllocator.create(&m_deviceContext);
		m_descriptorSetAllocator.create(&m_deviceContext);
		m_transferManager.create(&m_deviceContext, &m_resourceAllocator);
		m_pipelineLibrary.create(pipelineLibraryFileName, &m_deviceContext);

		m_context = { .deviceContext = &m_deviceContext,
					  .resourceAllocator = &m_resourceAllocator,
					  .descriptorSetAllocator = &m_descriptorSetAllocator,
					  .transferManager = &m_transferManager,
					  .pipelineLibrary = &m_pipelineLibrary,
					  .targetSurface = &m_renderTargetSurface };

		if (initFramegraph) {
			m_framegraphContext.create(m_context);
		}

		m_surface.create(m_deviceContext.instance(), frameInFlightCount);
	}

	bool GraphicsSubsystem::tickFrame() {
		++frameIndex %= frameInFlightCount;
		vkWaitForFences(m_deviceContext.device(), 1, &m_deviceContext.frameCompletionFence(frameIndex), VK_TRUE,
						UINT64_MAX);
		vkResetFences(m_deviceContext.device(), 1, &m_deviceContext.frameCompletionFence(frameIndex));

		if (m_surface.swapchainDirtyFlag()) {
			m_surface.createSwapchain(m_deviceContext.physicalDevice(), m_deviceContext.device(),
									  m_framegraphContext.targetImageUsageFlags());
		m_renderTargetSurface.create(m_surface.swapchainImages(m_deviceContext.device()),
									 { .width = m_surface.imageWidth(),
									   .height = m_surface.imageHeight(),
									   .format = m_surface.swapchainImageFormat() });
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

	GraphicsSubsystem::~GraphicsSubsystem() {
		vkDeviceWaitIdle(m_deviceContext.device());
		if (m_hasFramegraph)
			m_framegraphContext.destroy();
		m_pipelineLibrary.destroy();
		m_transferManager.destroy();
		m_descriptorSetAllocator.destroy();
		m_resourceAllocator.destroy();
		m_renderTargetSurface.destroy();
		m_surface.destroy(m_deviceContext.device(), m_deviceContext.instance());
		m_deviceContext.destroy();
	}
} // namespace vanadium::graphics
