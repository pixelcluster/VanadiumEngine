#include <modules/gpu/VGPUModule.hpp>
#include <modules/gpu/helper/ErrorHelper.hpp>
#include <volk.h>

VGPUModule::VGPUModule(const std::string_view& appName, uint32_t appVersion, VWindowModule* windowModule) {
	m_context.create(appName, appVersion, windowModule);
	m_windowModule = windowModule;

	VkSemaphoreCreateInfo semaphoreCreateInfo = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

	for (size_t i = 0; i < frameInFlightCount; ++i) {
		verifyResult(vkCreateSemaphore(m_context.device(), &semaphoreCreateInfo, nullptr, &m_signalSemaphores[i]));
	}
}

void VGPUModule::onCreate(VEngine& engine) {}

void VGPUModule::onActivate(VEngine& engine) {}

void VGPUModule::onExecute(VEngine& engine) {
	if (m_wasSwapchainInvalid || m_windowModule->wasResized()) {
		if (!m_context.recreateSwapchain(m_windowModule)) {
			m_windowModule->waitForEvents();
			return;
		}
	}

	AcquireResult result;
	if (m_wasSwapchainInvalid && m_invalidAcquiredResult.imageIndex) {
		result = m_invalidAcquiredResult;
		result.swapchainState = SwapchainState::OK; // don't fail right away
	} else {
		result = m_context.acquireImage();
	}

	if (result.swapchainState == SwapchainState::Invalid) {
		m_wasSwapchainInvalid = true;
		m_invalidAcquiredResult.imageIndex = -1U;
		return;
	}

	VkCommandBuffer frameCommandBuffer = m_context.frameCommandBuffer();
	VkCommandBufferBeginInfo beginInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
										   .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT };
	verifyResult(vkBeginCommandBuffer(frameCommandBuffer, &beginInfo));

	VkImageMemoryBarrier memoryBarrier = { .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
										   .srcAccessMask = 0,
										   .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
										   .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
										   .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
										   .image = m_context.swapchainImages()[result.imageIndex],
										   .subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
																 .baseMipLevel = 0,
																 .levelCount = 1,
																 .baseArrayLayer = 0,
																 .layerCount = 1 } };
	vkCmdPipelineBarrier(frameCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
						 nullptr, 0, nullptr, 1, &memoryBarrier);

	VkClearColorValue clearColor = { .float32 = { 0.2f, 0.2f, 0.2f, 1.0f } };

	VkImageSubresourceRange range = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
									  .baseMipLevel = 0,
									  .levelCount = 1,
									  .baseArrayLayer = 0,
									  .layerCount = 1 };

	vkCmdClearColorImage(frameCommandBuffer, m_context.swapchainImages()[result.imageIndex],
						 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor, 1, &range);

	memoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	memoryBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	memoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	memoryBarrier.dstAccessMask = 0;
	
	vkCmdPipelineBarrier(frameCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0,
						 nullptr, 0, nullptr, 1, &memoryBarrier);

	verifyResult(vkEndCommandBuffer(frameCommandBuffer));

	VkPipelineStageFlags semaphoreWaitStages = VK_PIPELINE_STAGE_TRANSFER_BIT;

	VkSubmitInfo submitInfo = { .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
								.waitSemaphoreCount = 1,
								.pWaitSemaphores = &result.imageSemaphore,
								.pWaitDstStageMask = &semaphoreWaitStages,
								.commandBufferCount = 1,
								.pCommandBuffers = &frameCommandBuffer,
								.signalSemaphoreCount = 1,
								.pSignalSemaphores = &m_signalSemaphores[result.frameIndex] };
	vkQueueSubmit(m_context.graphicsQueue(), 1, &submitInfo, m_context.frameCompletionFence());

	SwapchainState state = m_context.presentImage(result.imageIndex, m_signalSemaphores[result.frameIndex]);
	if (state == SwapchainState::Invalid) {
		m_wasSwapchainInvalid = true;
		m_invalidAcquiredResult = result;
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

	m_context.destroy();
}

void VGPUModule::onDependentModuleDeactivate(VEngine& engine, VModule* moduleToDestroy) {
	if (moduleToDestroy == m_windowModule)
		engine.setExitFlag();
}
