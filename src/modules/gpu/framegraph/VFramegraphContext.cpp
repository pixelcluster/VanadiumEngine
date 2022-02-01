#include <modules/gpu/framegraph/VFramegraphContext.hpp>
#include <modules/gpu/framegraph/VFramegraphNode.hpp>
#include <modules/gpu/helper/ErrorHelper.hpp>
#include <volk.h>

void VFramegraphContext::create(VGPUContext* context, VGPUResourceAllocator* resourceAllocator) {
	m_gpuContext = context;
	m_resourceAllocator = resourceAllocator;
}

void VFramegraphContext::setupResources() {
	m_nodeBufferDependencies.resize(m_nodes.size());
	m_nodeImageDependencies.resize(m_nodes.size());
	for (auto& node : m_nodes) {
		node.node->setupResources(this);
	}
}

void VFramegraphContext::declareCreatedBuffer(VFramegraphNode* creator, const std::string_view& name,
											  VImageResourceHandle handle, const VFramegraphNodeBufferUsage& usage) {
	m_buffers.insert(std::pair<std::string, VFramegraphBufferResource>(
		name, VFramegraphBufferResource{ .creator = creator, .bufferResourceHandle = handle, .usage = usage }));
}

void VFramegraphContext::declareCreatedImage(VFramegraphNode* creator, const std::string_view& name,
											 VImageResourceHandle handle, const VFramegraphNodeImageUsage& usage) {
	m_images.insert(std::pair<std::string, VFramegraphImageResource>(
		name, VFramegraphImageResource{ .creator = creator, .imageResourceHandle = handle, .usage = usage }));
	if (usage.viewInfo.has_value()) {
		auto nodeIterator =
			std::find_if(m_nodes.begin(), m_nodes.end(), [creator](const auto& info) { return info.node == creator; });
		if (nodeIterator == m_nodes.end()) {
			// TODO: log invalid node as creator
			return;
		}
		nodeIterator->resourceViewInfos.insert(
			std::pair<std::string, VImageResourceViewInfo>(name, usage.viewInfo.value()));
	}
}

void VFramegraphContext::declareReferencedBuffer(VFramegraphNode* user, const std::string_view& name,
												 const VFramegraphNodeBufferUsage& usage) {
	auto nodeIterator =
		std::find_if(m_nodes.begin(), m_nodes.end(), [user](const auto& info) { return info.node == user; });
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

	auto nodeIterator =
		std::find_if(m_nodes.begin(), m_nodes.end(), [user](const auto& info) { return info.node == user; });
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
												   .aspectFlags = usage.aspectFlags,
												   .srcStages = usageIterator->second.usage.pipelineStages,
												   .dstStages = usage.pipelineStages,
												   .srcAccesses = usageIterator->second.usage.accessTypes,
												   .dstAccesses = usage.accessTypes,
												   .oldLayout = usageIterator->second.usage.finishLayout,
												   .newLayout = usage.startLayout == VK_IMAGE_LAYOUT_UNDEFINED
																	? usageIterator->second.usage.finishLayout
																	: VK_IMAGE_LAYOUT_UNDEFINED });
	VkImageUsageFlags previousUsageFlags = usageIterator->second.usage.usageFlags;
	usageIterator->second.usage = usage;
	usageIterator->second.usage.usageFlags |= previousUsageFlags;

	if (usage.viewInfo.has_value()) {
		if (nodeIterator == m_nodes.end()) {
			// TODO: log invalid node as creator
			return;
		}
		nodeIterator->resourceViewInfos.insert(
			std::pair<std::string, VImageResourceViewInfo>(name, usage.viewInfo.value()));
	}
}

void VFramegraphContext::declareReferencedSwapchainImage(VFramegraphNode* user,
														 const VFramegraphNodeImageUsage& usage) {
	auto nodeIterator =
		std::find_if(m_nodes.begin(), m_nodes.end(), [user](const auto& info) { return info.node == user; });
	if (nodeIterator == m_nodes.end()) {
		// TODO: log invalid node for dependency
		return;
	}

	size_t nodeIndex = nodeIterator - m_nodes.begin();

	m_nodeSwapchainImageDependencies.insert(std::pair<size_t, VFramegraphImageDependency>(
		nodeIndex, { .srcStages = m_swapchainImageUsage.pipelineStages,
					 .dstStages = usage.pipelineStages,
					 .srcAccesses = m_swapchainImageUsage.accessTypes,
					 .dstAccesses = usage.accessTypes,
					 .oldLayout = m_swapchainImageUsage.finishLayout,
					 .newLayout = usage.startLayout == VK_IMAGE_LAYOUT_UNDEFINED ? m_swapchainImageUsage.finishLayout
																				 : usage.startLayout }));
	VkImageUsageFlags previousUsageFlags = m_swapchainImageUsage.usageFlags;
	m_swapchainImageUsage = usage;
	m_swapchainImageUsage.usageFlags |= previousUsageFlags;

	if (usage.viewInfo.has_value()) {
		if (nodeIterator == m_nodes.end()) {
			// TODO: log invalid node as creator
			return;
		}
		nodeIterator->swapchainResourceViewInfo = usage.viewInfo.value();
		m_swapchainImageViews[0].insert(
			std::pair<VImageResourceViewInfo, VkImageView>(usage.viewInfo.value(), VK_NULL_HANDLE));
	}
}

void VFramegraphContext::invalidateBuffer(const std::string& name, VBufferResourceHandle newHandle) {
	m_buffers[name].bufferResourceHandle = newHandle;
}

void VFramegraphContext::invalidateImage(const std::string& name, VImageResourceHandle newHandle) {
	m_images[name].imageResourceHandle = newHandle;
}

const VFramegraphBufferResource VFramegraphContext::bufferResource(const std::string& name) const {
	auto iterator = m_buffers.find(name);
	if (iterator == m_buffers.end())
		return {};
	else
		return iterator->second;
}

const VFramegraphImageResource VFramegraphContext::imageResource(const std::string& name) const {
	auto iterator = m_images.find(name);
	if (iterator == m_images.end())
		return {};
	else
		return iterator->second;
}

VkImageView VFramegraphContext::swapchainImageView(VFramegraphNode* node, uint32_t index) {
	auto nodeIterator =
		std::find_if(m_nodes.begin(), m_nodes.end(), [node](const auto& info) { return info.node == node; });
	if (nodeIterator == m_nodes.end()) {
		// TODO: log getting image view of unknown node
		return VK_NULL_HANDLE;
	}

	if (!nodeIterator->swapchainResourceViewInfo.has_value()) {
		// TODO: log getting swapchain image
		return VK_NULL_HANDLE;
	} else
		return m_swapchainImageViews[index][nodeIterator->swapchainResourceViewInfo.value()];
}

void VFramegraphContext::executeFrame(const AcquireResult& result, VkSemaphore signalSemaphore) {
	VkCommandBuffer frameCommandBuffer = m_gpuContext->frameCommandBuffer();
	VkCommandBufferBeginInfo beginInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
										   .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT };
	verifyResult(vkBeginCommandBuffer(frameCommandBuffer, &beginInfo));

	size_t nodeIndex = 0;
	std::vector<VkBufferMemoryBarrier> bufferMemoryBarriers;
	std::vector<VkImageMemoryBarrier> imageMemoryBarriers;

	VFramegraphNodeContext nodeContext = { .frameIndex = result.frameIndex,
										   .imageIndex = result.imageIndex,
										   .swapchainImage = m_gpuContext->swapchainImages()[result.imageIndex] };
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
			imageMemoryBarriers.push_back({ .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
											.srcAccessMask = dependency.srcAccesses,
											.dstAccessMask = dependency.dstAccesses,
											.oldLayout = dependency.oldLayout,
											.newLayout = dependency.newLayout,
											.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
											.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
											.image = nativeImageHandle(dependency.resourceName),
											.subresourceRange = { .aspectMask = dependency.aspectFlags,
																  .baseMipLevel = 0,
																  .levelCount = VK_REMAINING_MIP_LEVELS,
																  .baseArrayLayer = 0,
																  .layerCount = VK_REMAINING_ARRAY_LAYERS } });
			srcStageFlags |= dependency.srcStages;
			dstStageFlags |= dependency.dstStages;
		}

		auto swapchainDependencyIterator = m_nodeSwapchainImageDependencies.find(nodeIndex);
		if (swapchainDependencyIterator != m_nodeSwapchainImageDependencies.end()) {
			imageMemoryBarriers.push_back({ .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
											.srcAccessMask = swapchainDependencyIterator->second.srcAccesses,
											.dstAccessMask = swapchainDependencyIterator->second.dstAccesses,
											.oldLayout = swapchainDependencyIterator->second.oldLayout,
											.newLayout = swapchainDependencyIterator->second.newLayout,
											.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
											.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
											.image = nodeContext.swapchainImage,
											.subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
																  .baseMipLevel = 0,
																  .levelCount = VK_REMAINING_MIP_LEVELS,
																  .baseArrayLayer = 0,
																  .layerCount = VK_REMAINING_ARRAY_LAYERS } });
			srcStageFlags |= swapchainDependencyIterator->second.srcStages;
			dstStageFlags |= swapchainDependencyIterator->second.dstStages;
		}

		if (!bufferMemoryBarriers.empty() || !imageMemoryBarriers.empty()) {
			vkCmdPipelineBarrier(frameCommandBuffer, srcStageFlags, dstStageFlags, 0, 0, nullptr,
								 static_cast<uint32_t>(bufferMemoryBarriers.size()), bufferMemoryBarriers.data(),
								 static_cast<uint32_t>(imageMemoryBarriers.size()), imageMemoryBarriers.data());
		}

		for (auto& viewInfo : node.resourceViewInfos) {
			VkImageView view =
				m_resourceAllocator->requestImageView(m_images[viewInfo.first].imageResourceHandle, viewInfo.second);
			nodeContext.resourceImageViews.insert(std::pair<std::string, VkImageView>(viewInfo.first, view));
		}

		if (node.swapchainResourceViewInfo.has_value()) {
			nodeContext.swapchainImageView =
				m_swapchainImageViews[result.imageIndex][node.swapchainResourceViewInfo.value()];
		}

		node.node->recordCommands(this, frameCommandBuffer, nodeContext);

		bufferMemoryBarriers.clear();
		imageMemoryBarriers.clear();
		nodeContext.resourceImageViews.clear();
		++nodeIndex;
	}

	if (m_swapchainImageUsage.finishLayout != VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
		VkImageMemoryBarrier transitionBarrier = { .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
												   .srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT,
												   .dstAccessMask = 0,
												   .oldLayout = m_swapchainImageUsage.finishLayout,
												   .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
												   .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
												   .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
												   .image = m_gpuContext->swapchainImages()[result.imageIndex],
												   .subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
																		 .baseMipLevel = 0,
																		 .levelCount = VK_REMAINING_MIP_LEVELS,
																		 .baseArrayLayer = 0,
																		 .layerCount = VK_REMAINING_ARRAY_LAYERS } };

		vkCmdPipelineBarrier(frameCommandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
							 VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &transitionBarrier);
	}

	verifyResult(vkEndCommandBuffer(frameCommandBuffer));

	VkPipelineStageFlags semaphoreWaitStages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

	VkSubmitInfo submitInfo = { .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
								.waitSemaphoreCount = 1,
								.pWaitSemaphores = &result.imageSemaphore,
								.pWaitDstStageMask = &semaphoreWaitStages,
								.commandBufferCount = 1,
								.pCommandBuffers = &frameCommandBuffer,
								.signalSemaphoreCount = 1,
								.pSignalSemaphores = &signalSemaphore };
	vkQueueSubmit(m_gpuContext->graphicsQueue(), 1, &submitInfo, m_gpuContext->frameCompletionFence());
}

void VFramegraphContext::handleSwapchainResize(uint32_t width, uint32_t height) {
	uint32_t imageIndex = 0;
	for (auto& image : m_swapchainImageViews) {
		for (auto& viewInfo : image) {
			vkDestroyImageView(m_gpuContext->device(), viewInfo.second, nullptr);
		}
	}

	m_swapchainImageViews.resize(m_gpuContext->swapchainImages().size());

	for (size_t i = 0; i < m_swapchainImageViews.size(); ++i) {
		m_swapchainImageViews[i] = m_swapchainImageViews[0];

		auto& imageViewMap = m_swapchainImageViews[i];
		for (auto& viewInfo : imageViewMap) {
			VkImageViewCreateInfo createInfo = { .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
												 .flags = viewInfo.first.flags,
												 .image = m_gpuContext->swapchainImages()[imageIndex],
												 .viewType = viewInfo.first.viewType,
												 .format = m_gpuContext->swapchainImageFormat(),
												 .components = viewInfo.first.components,
												 .subresourceRange = viewInfo.first.subresourceRange };
			vkCreateImageView(m_gpuContext->device(), &createInfo, nullptr, &viewInfo.second);
		}
	}

	for (auto& node : m_nodes) {
		node.node->handleWindowResize(this, width, height);
	}
}
