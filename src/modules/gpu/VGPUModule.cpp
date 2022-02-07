#include <modules/gpu/VGPUModule.hpp>
#include <modules/gpu/helper/ErrorHelper.hpp>
#include <volk.h>

VGPUModule::VGPUModule(const std::string_view& appName, uint32_t appVersion, VWindowModule* windowModule) {
	m_context.create(appName, appVersion, windowModule);
	m_windowModule = windowModule;

	m_resourceAllocator.create(&m_context);
	m_transferManager.create(&m_context, &m_resourceAllocator);
	m_descriptorSetAllocator.create(&m_context);
	m_framegraphContext.create(&m_context, &m_resourceAllocator, &m_descriptorSetAllocator, &m_transferManager);

	VkSemaphoreCreateInfo semaphoreCreateInfo = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

	for (size_t i = 0; i < frameInFlightCount; ++i) {
		verifyResult(vkCreateSemaphore(m_context.device(), &semaphoreCreateInfo, nullptr, &m_signalSemaphores[i]));
	}
}

void VGPUModule::onCreate(VEngine& engine) {  }

void VGPUModule::onActivate(VEngine& engine) {
	m_framegraphContext.setupResources();
	m_context.recreateSwapchain(m_windowModule, m_framegraphContext.swapchainImageUsageFlags());
}

void VGPUModule::onExecute(VEngine& engine) {
	m_memoryBudgetTimer += m_windowModule->deltaTime();
	if (m_memoryBudgetTimer > 1.0) {
		m_memoryBudgetTimer -= 1.0;
		m_resourceAllocator.updateMemoryBudget();
	}

	if (m_wasSwapchainInvalid || m_windowModule->wasResized()) {
		if (!m_context.recreateSwapchain(m_windowModule, m_framegraphContext.swapchainImageUsageFlags())) {
			m_windowModule->waitForEvents();
			return;
		}
		m_framegraphContext.handleSwapchainResize(m_windowModule->width(), m_windowModule->height());
	}

	AcquireResult result;
	result = m_context.acquireImage();

	if (result.swapchainState == SwapchainState::Invalid) {
		m_wasSwapchainInvalid = true;
		return;
	}

	m_resourceAllocator.setFrameIndex(result.frameIndex);

	VkCommandBuffer framegraphCommandBuffer = m_framegraphContext.recordFrame(result);
	VkCommandBuffer transferCommandBuffer = m_transferManager.recordTransfers(result.frameIndex);

	VkCommandBuffer commandBuffers[] = { transferCommandBuffer, framegraphCommandBuffer };

	VkPipelineStageFlags semaphoreWaitStages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

	VkSubmitInfo submitInfo = { .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
								.waitSemaphoreCount = 1,
								.pWaitSemaphores = &result.imageSemaphore,
								.pWaitDstStageMask = &semaphoreWaitStages,
								.commandBufferCount = 2,
								.pCommandBuffers = commandBuffers,
								.signalSemaphoreCount = 1,
								.pSignalSemaphores = &m_signalSemaphores[result.imageIndex] };
	vkQueueSubmit(m_context.graphicsQueue(), 1, &submitInfo, m_context.frameCompletionFence());

	SwapchainState state = m_context.presentImage(result.imageIndex, m_signalSemaphores[result.imageIndex]);
	if (state == SwapchainState::Invalid) {
		m_wasSwapchainInvalid = true;
		return;
	}

	m_wasSwapchainInvalid = false;
}

void VGPUModule::onDeactivate(VEngine& engine) {}

void VGPUModule::onDestroy(VEngine& engine) {
	vkDeviceWaitIdle(m_context.device());
	for (auto& semaphore : m_signalSemaphores) {
		vkDestroySemaphore(m_context.device(), semaphore, nullptr);
	}
	m_framegraphContext.destroy();
	m_transferManager.destroy();
	m_resourceAllocator.destroy();
	m_context.destroy();
}

void VGPUModule::onDependentModuleDeactivate(VEngine& engine, VModule* moduleToDestroy) {
	if (moduleToDestroy == m_windowModule)
		engine.setExitFlag();
}
