#include <modules/gpu/framegraph/VFramegraphContext.hpp>
#include <modules/gpu/framegraph/VFramegraphNode.hpp>
#include <modules/gpu/helper/ErrorHelper.hpp>
#include <volk.h>

void VFramegraphContext::create(VGPUContext* context, VGPUResourceAllocator* resourceAllocator) {
	m_gpuContext = context;
	m_resourceAllocator = resourceAllocator;

	VImageResourceHandle swapchainImageHandle = resourceAllocator->createExternalImage(VK_NULL_HANDLE);

	declareImportedImage("Swapchain image", swapchainImageHandle,
						 { .pipelineStages = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
						   .accessTypes = 0,
						   .finishLayout = VK_IMAGE_LAYOUT_UNDEFINED });
}

void VFramegraphContext::setupResources() {
	for (auto& node : m_nodes) {
		node->setupResources(this);
	}
}

void VFramegraphContext::declareCreatedBuffer(VFramegraphNode* creator, const std::string_view& name,
											  VImageResourceHandle handle,
											  const VFramegraphNodeBufferUsage& usage) {
	m_buffers.insert(std::pair<std::string, VFramegraphBufferResource>(
		name, VFramegraphBufferResource{ .creator = creator, .bufferResourceHandle = handle, .usage = usage }));
}

void VFramegraphContext::declareCreatedImage(VFramegraphNode* creator, const std::string_view& name,
											 VImageResourceHandle handle,
											 const VFramegraphNodeImageUsage& usage) {
	m_images.insert(std::pair<std::string, VFramegraphImageResource>(
		name, VFramegraphImageResource{ .creator = creator, .imageResourceHandle = handle, .usage = usage }));
}

void VFramegraphContext::declareImportedBuffer(const std::string_view& name, VBufferResourceHandle handle,
											   const VFramegraphNodeBufferUsage& usage) {
	m_buffers.insert(std::pair<std::string, VFramegraphBufferResource>(
		name, VFramegraphBufferResource{ .bufferResourceHandle = handle, .usage = usage }));
}

void VFramegraphContext::declareImportedImage(const std::string_view& name, VImageResourceHandle handle,
											  const VFramegraphNodeImageUsage& usage) {
	m_images.insert(std::pair<std::string, VFramegraphImageResource>(
		name, VFramegraphImageResource{ .imageResourceHandle = handle, .usage = usage }));
}

void VFramegraphContext::declareReferencedBuffer(VFramegraphNode* user, const std::string_view& name,
												 const VFramegraphNodeBufferUsage& usage) {
	m_nodeBufferDependencies.resize(m_nodes.size());
	m_nodeImageDependencies.resize(m_nodes.size());

	auto nodeIterator = std::find(m_nodes.begin(), m_nodes.end(), user);
	if (nodeIterator == m_nodes.end()) {
		// TODO: log invalid node for dependency
		return;
	}

	auto usageIterator = m_buffers.find(std::string(name));
	if (usageIterator == m_buffers.end()) {
		// TODO: log invalid resource for dependency
		return;
	}

	size_t nodeIndex = nodeIterator - m_nodes.begin();

	m_nodeBufferDependencies[nodeIndex].push_back({ .resourceName = std::string(name),
													.srcStages = usageIterator->second.usage.pipelineStages,
													.dstStages = usage.pipelineStages,
													.srcAccesses = usageIterator->second.usage.accessTypes,
													.dstAccesses = usage.accessTypes });

	VkBufferUsageFlags previousUsageFlags = usageIterator->second.usage.usageFlags;
	usageIterator->second.usage = usage;
	usageIterator->second.usage.usageFlags |= previousUsageFlags;
}

void VFramegraphContext::declareReferencedImage(VFramegraphNode* user, const std::string_view& name,
												const VFramegraphNodeImageUsage& usage) {
	m_nodeBufferDependencies.resize(m_nodes.size());
	m_nodeImageDependencies.resize(m_nodes.size());

	auto nodeIterator = std::find(m_nodes.begin(), m_nodes.end(), user);
	if (nodeIterator == m_nodes.end()) {
		// TODO: log invalid node for dependency
		return;
	}

	auto usageIterator = m_images.find(std::string(name));
	if (usageIterator == m_images.end()) {
		// TODO: log invalid resource for dependency
		return;
	}

	size_t nodeIndex = nodeIterator - m_nodes.begin();

	m_nodeImageDependencies[nodeIndex].push_back({ .resourceName = std::string(name),
												   .srcStages = usageIterator->second.usage.pipelineStages,
												   .dstStages = usage.pipelineStages,
												   .srcAccesses = usageIterator->second.usage.accessTypes,
												   .dstAccesses = usage.accessTypes,
												   .oldLayout = usageIterator->second.usage.finishLayout,
												   .newLayout = usage.startLayout });
	VkImageUsageFlags previousUsageFlags = usageIterator->second.usage.usageFlags;
	usageIterator->second.usage = usage;
	usageIterator->second.usage.usageFlags |= previousUsageFlags;
}

void VFramegraphContext::executeFrame(const AcquireResult& result, VkSemaphore signalSemaphore) {
	m_nodeBufferDependencies.resize(m_nodes.size());
	m_nodeImageDependencies.resize(m_nodes.size());

	m_resourceAllocator->updateExternalImage(m_images["Swapchain image"].imageResourceHandle,
											 m_gpuContext->swapchainImages()[result.imageIndex]);

	VkCommandBuffer frameCommandBuffer = m_gpuContext->frameCommandBuffer();
	VkCommandBufferBeginInfo beginInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
										   .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT };
	verifyResult(vkBeginCommandBuffer(frameCommandBuffer, &beginInfo));

	size_t nodeIndex = 0;
	std::vector<VkBufferMemoryBarrier> bufferMemoryBarriers;
	std::vector<VkImageMemoryBarrier> imageMemoryBarriers;
	for (auto& node : m_nodes) {
		bufferMemoryBarriers.reserve(m_nodeBufferDependencies[nodeIndex].size());
		imageMemoryBarriers.reserve(m_nodeImageDependencies[nodeIndex].size());

		VkPipelineStageFlags srcStageFlags = 0;
		VkPipelineStageFlags dstStageFlags = 0;

		for (auto& dependency : m_nodeBufferDependencies[nodeIndex]) {
			bufferMemoryBarriers.push_back({ .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
											 .srcAccessMask = dependency.srcAccesses,
											 .dstAccessMask = dependency.dstAccesses,
											 .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
											 .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
											 .buffer = nativeBufferHandle(dependency.resourceName),
											 .offset = 0U,
											 .size = VK_WHOLE_SIZE });
			srcStageFlags |= dependency.srcStages;
			dstStageFlags |= dependency.dstStages;
		}
		for (auto& dependency : m_nodeImageDependencies[nodeIndex]) {
			imageMemoryBarriers.push_back(
				{ .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				  .srcAccessMask = dependency.srcAccesses,
				  .dstAccessMask = dependency.dstAccesses,
				  .oldLayout = dependency.oldLayout,
				  .newLayout = dependency.newLayout,
				  .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				  .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				  .image = nativeImageHandle(dependency.resourceName),
				  .subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT | VK_IMAGE_ASPECT_DEPTH_BIT,
										.baseMipLevel = 0,
										.levelCount = VK_REMAINING_MIP_LEVELS,
										.baseArrayLayer = 0,
										.layerCount = VK_REMAINING_ARRAY_LAYERS } });
			srcStageFlags |= dependency.srcStages;
			dstStageFlags |= dependency.dstStages;
		}

		if (!bufferMemoryBarriers.empty() || !imageMemoryBarriers.empty()) {
			vkCmdPipelineBarrier(frameCommandBuffer, srcStageFlags, dstStageFlags, 0, 0, nullptr,
								 static_cast<uint32_t>(bufferMemoryBarriers.size()), bufferMemoryBarriers.data(),
								 static_cast<uint32_t>(imageMemoryBarriers.size()), imageMemoryBarriers.data());
		}

		node->recordCommands(this, frameCommandBuffer);

		bufferMemoryBarriers.clear();
		imageMemoryBarriers.clear();
		++nodeIndex;
	}

	if (m_images["Swapchain image"].usage.finishLayout != VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
		VkImageMemoryBarrier transitionBarrier = { .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
												   .srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT,
												   .dstAccessMask = 0,
												   .oldLayout = m_images["Swapchain image"].usage.finishLayout,
												   .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
												   .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
												   .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
												   .image = m_gpuContext->swapchainImages()[result.imageIndex],
												   .subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT |
																					   VK_IMAGE_ASPECT_DEPTH_BIT,
																		 .baseMipLevel = 0,
																		 .levelCount = VK_REMAINING_MIP_LEVELS,
																		 .baseArrayLayer = 0,
																		 .layerCount = VK_REMAINING_ARRAY_LAYERS } };

		vkCmdPipelineBarrier(frameCommandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
							 VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &transitionBarrier);
	}

	verifyResult(vkEndCommandBuffer(frameCommandBuffer));

	VkSubmitInfo submitInfo = { .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
								.waitSemaphoreCount = 1,
								.pWaitSemaphores = &result.imageSemaphore,
								.commandBufferCount = 1,
								.pCommandBuffers = &frameCommandBuffer,
								.signalSemaphoreCount = 1,
								.pSignalSemaphores = &signalSemaphore };
	vkQueueSubmit(m_gpuContext->graphicsQueue(), 1, &submitInfo, m_gpuContext->frameCompletionFence());
}
