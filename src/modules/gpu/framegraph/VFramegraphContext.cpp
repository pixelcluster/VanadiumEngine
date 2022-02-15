#include <VDebug.hpp>
#include <cstdio>
#include <modules/gpu/framegraph/VFramegraphContext.hpp>
#include <modules/gpu/framegraph/VFramegraphNode.hpp>
#include <modules/gpu/helper/DebugHelper.hpp>
#include <modules/gpu/helper/ErrorHelper.hpp>
#include <volk.h>

// true if [offset1; offset1 + size1] overlaps with [offset2; offset2 + size2]
template <typename T, T fullRangeValue> bool overlaps(T offset1, T size1, T offset2, T size2) {
	if (size1 == fullRangeValue) {
		if (size2 == fullRangeValue) {
			return true;
		} else {
			return offset1 < offset2 + size2;
		}
	} else if (size2 == fullRangeValue) {
		return offset1 + size1 > offset2;
	} else {
		return offset1 >= offset2 && offset1 < offset2 + size2;
	}
}

// true if [offset2; offset2 + size2] is completely contained within [offset1; offset1 + size1]
template <typename T, T fullRangeValue> bool isCompletelyContained(T offset1, T size1, T offset2, T size2) {
	if (size1 == fullRangeValue) {
		if (size2 == fullRangeValue) {
			return true;
		} else {
			return offset1 <= offset2;
		}
	} else if (size2 == fullRangeValue) {
		return false;
	} else {
		return offset1 <= offset2 && offset1 + size1 >= offset2 + size2;
	}
}

void VFramegraphContext::create(VGPUContext* context, VGPUResourceAllocator* resourceAllocator,
								VGPUDescriptorSetAllocator* descriptorSetAllocator,
								VGPUTransferManager* transferManager) {
	m_gpuContext = context;
	m_resourceAllocator = resourceAllocator;
	m_descriptorSetAllocator = descriptorSetAllocator;
	m_transferManager = transferManager;

	for (size_t i = 0; i < frameInFlightCount; ++i) {
		VkCommandBufferAllocateInfo allocateInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
													 .commandPool = m_gpuContext->perFrameCommandPool(i),
													 .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
													 .commandBufferCount = 1 };
		verifyResult(vkAllocateCommandBuffers(m_gpuContext->device(), &allocateInfo, &m_frameCommandBuffers[i]));
	}
}

void VFramegraphContext::setupResources() {
	m_nodeBufferMemoryBarriers.resize(m_nodes.size());
	m_nodeImageMemoryBarriers.resize(m_nodes.size());
	m_nodeBarrierStages.resize(m_nodes.size());

	for (auto& node : m_nodes) {
		node.node->setupResources(this);
	}

	for (auto handle : m_transientBuffers) {
		createBuffer(handle);
	}
	for (auto handle : m_transientImages) {
		createImage(handle);
	}

	for (auto& node : m_nodes) {
		for (auto& info : node.resourceViewInfos) {
			m_resourceAllocator->requestImageView(m_images[info.first].resourceHandle, info.second);
		}
		node.node->initResources(this);
	}

	updateDependencyInfo();
}

VFramegraphBufferHandle VFramegraphContext::declareTransientBuffer(
	VFramegraphNode* creator, const VFramegraphBufferCreationParameters& parameters,
	const VFramegraphNodeBufferUsage& usage) {
	VFramegraphBufferHandle handle = m_buffers.addElement(VFramegraphBufferResource{.creationParameters = parameters});
	m_transientBuffers.push_back(handle);

	auto nodeIterator =
		std::find_if(m_nodes.begin(), m_nodes.end(), [creator](const auto& info) { return info.node == creator; });
	if (nodeIterator == m_nodes.end()) {
		printf("invalid node as creator!\n");
		return { ~0U };
	}

	auto usageIterator = m_buffers.find(handle);
	if (usageIterator == m_buffers.end()) {
		printf("invalid resource for dependency!\n");
		return { ~0U };
	}

	size_t nodeIndex = nodeIterator - m_nodes.begin();

	addUsage(handle, nodeIndex, usageIterator->modifications, usageIterator->reads, usage);
	return handle;
}

VFramegraphImageHandle VFramegraphContext::declareTransientImage(VFramegraphNode* creator,
																 const VFramegraphImageCreationParameters& parameters,
																 const VFramegraphNodeImageUsage& usage) {
	VFramegraphImageHandle handle = m_images.addElement(VFramegraphImageResource{ .creationParameters = parameters });
	m_transientImages.push_back(handle);

	auto nodeIterator =
		std::find_if(m_nodes.begin(), m_nodes.end(), [creator](const auto& info) { return info.node == creator; });
	if (nodeIterator == m_nodes.end()) {
		printf("invalid node as creator!\n");
		return { ~0U };
	}
	size_t nodeIndex = nodeIterator - m_nodes.begin();

	auto usageIterator = m_images.find(handle);
	addUsage(handle, nodeIndex, usageIterator->modifications, usageIterator->reads, usage);

	if (usage.viewInfo.has_value()) {
		nodeIterator->resourceViewInfos.insert(
			std::pair<VFramegraphImageHandle, VImageResourceViewInfo>(handle, usage.viewInfo.value()));
	}
	return handle;
}

VFramegraphBufferHandle VFramegraphContext::declareImportedBuffer(VFramegraphNode* creator,
																  VBufferResourceHandle handle,
																  const VFramegraphNodeBufferUsage& usage) {

	VFramegraphBufferHandle bufferHandle = m_buffers.addElement(VFramegraphBufferResource{ .resourceHandle = handle });

	auto nodeIterator =
		std::find_if(m_nodes.begin(), m_nodes.end(), [creator](const auto& info) { return info.node == creator; });
	if (nodeIterator == m_nodes.end()) {
		printf("invalid node as creator!\n");
		return { ~0U };
	}

	auto usageIterator = m_buffers.find(bufferHandle);
	if (usageIterator == m_buffers.end()) {
		printf("invalid resource for dependency!\n");
		return { ~0U };
	}

	size_t nodeIndex = nodeIterator - m_nodes.begin();

	addUsage(bufferHandle, nodeIndex, usageIterator->modifications, usageIterator->reads, usage);

	return bufferHandle;
}

VFramegraphImageHandle VFramegraphContext::declareImportedImage(VFramegraphNode* creator, VImageResourceHandle handle,
																const VFramegraphNodeImageUsage& usage) {
	VFramegraphImageHandle imageHandle = m_images.addElement(VFramegraphImageResource{ .resourceHandle = handle });

	auto nodeIterator =
		std::find_if(m_nodes.begin(), m_nodes.end(), [creator](const auto& info) { return info.node == creator; });
	if (nodeIterator == m_nodes.end()) {
		printf("invalid node as creator!\n");
		return { ~0U };
	}
	size_t nodeIndex = nodeIterator - m_nodes.begin();

	auto usageIterator = m_images.find(imageHandle);
	addUsage(imageHandle, nodeIndex, usageIterator->modifications, usageIterator->reads, usage);

	if (usage.viewInfo.has_value()) {
		nodeIterator->resourceViewInfos.insert(
			std::pair<VFramegraphImageHandle, VImageResourceViewInfo>(imageHandle, usage.viewInfo.value()));
	}
	return imageHandle;
}

void VFramegraphContext::declareReferencedBuffer(VFramegraphNode* user, VFramegraphBufferHandle handle,
												 const VFramegraphNodeBufferUsage& usage) {
	auto nodeIterator =
		std::find_if(m_nodes.begin(), m_nodes.end(), [user](const auto& info) { return info.node == user; });
	if (nodeIterator == m_nodes.end()) {
		printf("invalid node for dependency!\n");
		return;
	}

	auto usageIterator = m_buffers.find(handle);
	if (usageIterator == m_buffers.end()) {
		printf("invalid resource for dependency!\n");
		return;
	}

	size_t nodeIndex = nodeIterator - m_nodes.begin();

	addUsage(handle, nodeIndex, usageIterator->modifications, usageIterator->reads, usage);
}

void VFramegraphContext::declareReferencedImage(VFramegraphNode* user, VFramegraphImageHandle handle,
												const VFramegraphNodeImageUsage& usage) {

	auto nodeIterator =
		std::find_if(m_nodes.begin(), m_nodes.end(), [user](const auto& info) { return info.node == user; });
	if (nodeIterator == m_nodes.end()) {
		printf("invalid node for dependency!\n");
		return;
	}

	auto usageIterator = m_images.find(handle);
	if (usageIterator == m_images.end()) {
		printf("invalid resource for dependency!\n");
		return;
	}

	size_t nodeIndex = nodeIterator - m_nodes.begin();

	addUsage(handle, nodeIndex, usageIterator->modifications, usageIterator->reads, usage);

	if (usage.viewInfo.has_value()) {
		if (nodeIterator == m_nodes.end()) {
			printf("invalid node as creator!\n");
			return;
		}
		nodeIterator->resourceViewInfos.insert(
			std::pair<VFramegraphImageHandle, VImageResourceViewInfo>(handle, usage.viewInfo.value()));
	}
}

void VFramegraphContext::declareReferencedSwapchainImage(VFramegraphNode* user,
														 const VFramegraphNodeImageUsage& usage) {
	auto nodeIterator =
		std::find_if(m_nodes.begin(), m_nodes.end(), [user](const auto& info) { return info.node == user; });
	if (nodeIterator == m_nodes.end()) {
		printf("invalid node for dependency!\n");
		return;
	}

	size_t nodeIndex = nodeIterator - m_nodes.begin();

	addUsage(nodeIndex, m_swapchainImageModifications, m_swapchainImageReads, usage);
	m_swapchainImageUsageFlags |= usage.usageFlags;

	if (usage.viewInfo.has_value()) {
		if (nodeIterator == m_nodes.end()) {
			printf("invalid node as creator!\n");
			return;
		}
		nodeIterator->swapchainResourceViewInfo = usage.viewInfo.value();
		m_swapchainImageViews[0].insert(
			std::pair<VImageResourceViewInfo, VkImageView>(usage.viewInfo.value(), VK_NULL_HANDLE));
	}
}

void VFramegraphContext::invalidateBuffer(VFramegraphBufferHandle handle, VBufferResourceHandle newHandle) {
	m_buffers[handle].resourceHandle = newHandle;
}

void VFramegraphContext::invalidateImage(VFramegraphImageHandle handle, VImageResourceHandle newHandle) {
	m_images[handle].resourceHandle = newHandle;
}

VFramegraphBufferResource VFramegraphContext::bufferResource(VFramegraphBufferHandle handle) const {
	auto iterator = m_buffers.find(handle);
	if (iterator == m_buffers.cend())
		return {};
	else
		return *iterator;
}

VFramegraphImageResource VFramegraphContext::imageResource(VFramegraphImageHandle handle) const {
	auto iterator = m_images.find(handle);
	if (iterator == m_images.cend())
		return {};
	else
		return *iterator;
}

void VFramegraphContext::recreateBufferResource(VFramegraphBufferHandle handle,
												const VFramegraphBufferCreationParameters& parameters) {
	if constexpr (vanadiumDebug) {
		if (std::find(m_transientBuffers.begin(), m_transientBuffers.end(), handle) == m_transientBuffers.end()) {
			printf("Trying to recreate non-created buffer!");
		}
	}
	m_buffers[handle].creationParameters = parameters;
	createBuffer(handle);
}

void VFramegraphContext::recreateImageResource(VFramegraphImageHandle handle,
											   const VFramegraphImageCreationParameters& parameters) {
	if constexpr (vanadiumDebug) {
		if (std::find(m_transientImages.begin(), m_transientImages.end(), handle) == m_transientImages.end()) {
			printf("Trying to recreate non-created image!");
		}
	}
	m_images[handle].creationParameters = parameters;
	createImage(handle);
}

VkBuffer VFramegraphContext::nativeBufferHandle(VFramegraphBufferHandle handle) {
	if constexpr (vanadiumDebug) {
		if (m_buffers.find(handle) == m_buffers.end()) {
			printf("Trying to get handle of non-created buffer");
		}
	}
	return m_resourceAllocator->nativeBufferHandle(m_buffers[handle].resourceHandle);
}

VkImage VFramegraphContext::nativeImageHandle(VFramegraphImageHandle handle) {
	if constexpr (vanadiumDebug) {
		if (m_images.find(handle) == m_images.end()) {
			printf("Trying to get handle of non-created image!");
		}
	}
	return m_resourceAllocator->nativeImageHandle(m_images[handle].resourceHandle);
}

VkImageView VFramegraphContext::imageView(VFramegraphNode* node, VFramegraphImageHandle handle) {
	auto nodeIterator =
		std::find_if(m_nodes.begin(), m_nodes.end(), [node](const auto& info) { return info.node == node; });
	if (nodeIterator == m_nodes.end()) {
		printf("invalid node for dependency!\n");
		return VK_NULL_HANDLE;
	}

	size_t nodeIndex = nodeIterator - m_nodes.begin();
	return m_resourceAllocator->requestImageView(m_images[handle].resourceHandle, nodeIterator->resourceViewInfos[handle]);
}

VkImageView VFramegraphContext::swapchainImageView(VFramegraphNode* node, uint32_t index) {
	auto nodeIterator =
		std::find_if(m_nodes.begin(), m_nodes.end(), [node](const auto& info) { return info.node == node; });
	if (nodeIterator == m_nodes.end()) {
		printf("getting image view of unknown node!\n");
		return VK_NULL_HANDLE;
	}

	if (!nodeIterator->swapchainResourceViewInfo.has_value()) {
		printf("getting swapchain image view of node that doesn't use swapchain images!\n");
		return VK_NULL_HANDLE;
	} else
		return m_swapchainImageViews[index][nodeIterator->swapchainResourceViewInfo.value()];
}

VkBufferUsageFlags VFramegraphContext::bufferUsageFlags(VFramegraphBufferHandle handle) {
	return m_buffers[handle].usageFlags;
}

VkImageUsageFlags VFramegraphContext::imageUsageFlags(VFramegraphImageHandle handle) { return m_images[handle].usage; }

VkCommandBuffer VFramegraphContext::recordFrame(const AcquireResult& result) {
	updateBarriers(result.imageIndex);

	VkCommandBuffer frameCommandBuffer = m_frameCommandBuffers[result.frameIndex];

	VkCommandBufferBeginInfo beginInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
										   .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT };
	verifyResult(vkBeginCommandBuffer(frameCommandBuffer, &beginInfo));

	size_t nodeIndex = 0;

	VFramegraphNodeContext nodeContext = { .frameIndex = result.frameIndex,
										   .imageIndex = result.imageIndex,
										   .swapchainImage = m_gpuContext->swapchainImages()[result.imageIndex] };

	if (m_firstFrameFlag) {
		if (!m_initImageMemoryBarriers.empty()) {
			vkCmdPipelineBarrier(frameCommandBuffer, m_initBarrierStages.src, m_initBarrierStages.dst, 0, 0, nullptr, 0,
								 nullptr, static_cast<uint32_t>(m_initImageMemoryBarriers.size()),
								 m_initImageMemoryBarriers.data());
			m_initImageMemoryBarriers.clear();
		}
		m_firstFrameFlag = false;
	} else if (!m_frameStartImageMemoryBarriers.empty() || !m_frameStartBufferMemoryBarriers.empty()) {
		vkCmdPipelineBarrier(
			frameCommandBuffer, m_frameStartBarrierStages.src, m_frameStartBarrierStages.dst, 0, 0, nullptr,
			static_cast<uint32_t>(m_frameStartBufferMemoryBarriers.size()), m_frameStartBufferMemoryBarriers.data(),
			static_cast<uint32_t>(m_frameStartImageMemoryBarriers.size()), m_frameStartImageMemoryBarriers.data());
	}

	for (auto& node : m_nodes) {
		if constexpr (vanadiumGPUDebug) {
			VkDebugUtilsLabelEXT label = { .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
										   .pLabelName = node.node->name().c_str() };
			vkCmdBeginDebugUtilsLabelEXT(frameCommandBuffer, &label);
		}

		if (!m_nodeBufferMemoryBarriers[nodeIndex].empty() || !m_nodeImageMemoryBarriers[nodeIndex].empty()) {
			vkCmdPipelineBarrier(frameCommandBuffer, m_nodeBarrierStages[nodeIndex].src,
								 m_nodeBarrierStages[nodeIndex].dst, 0, 0, nullptr,
								 static_cast<uint32_t>(m_nodeBufferMemoryBarriers[nodeIndex].size()),
								 m_nodeBufferMemoryBarriers[nodeIndex].data(),
								 static_cast<uint32_t>(m_nodeImageMemoryBarriers[nodeIndex].size()),
								 m_nodeImageMemoryBarriers[nodeIndex].data());
		}

		for (auto& viewInfo : node.resourceViewInfos) {
			VkImageView view =
				m_resourceAllocator->requestImageView(m_images[viewInfo.first].resourceHandle, viewInfo.second);
			nodeContext.resourceImageViews.insert(std::pair<VFramegraphImageHandle, VkImageView>(viewInfo.first, view));
		}

		if (node.swapchainResourceViewInfo.has_value()) {
			nodeContext.swapchainImageView =
				m_swapchainImageViews[result.imageIndex][node.swapchainResourceViewInfo.value()];
		}

		node.node->recordCommands(this, frameCommandBuffer, nodeContext);

		if constexpr (vanadiumGPUDebug) {
			vkCmdEndDebugUtilsLabelEXT(frameCommandBuffer);
		}

		nodeContext.resourceImageViews.clear();
		++nodeIndex;
	}

	VkImageLayout lastSwapchainImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	bool isLastAccessRead = false;
	VkImageSubresourceRange accessRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
											.baseMipLevel = 0,
											.levelCount = 1,
											.baseArrayLayer = 0,
											.layerCount = 1 };
	if (!m_swapchainImageReads.empty() && !m_swapchainImageModifications.empty()) {
		isLastAccessRead =
			m_swapchainImageReads.back().usingNodeIndex > m_swapchainImageModifications.back().usingNodeIndex;
	} else {
		isLastAccessRead = m_swapchainImageModifications.empty();
	}
	if (isLastAccessRead) {
		lastSwapchainImageLayout = m_swapchainImageReads.back().finishLayout;
		accessRange = m_swapchainImageReads.back().subresourceRange;
	} else if (!m_swapchainImageModifications.empty()) {
		lastSwapchainImageLayout = m_swapchainImageModifications.back().finishLayout;
		accessRange = m_swapchainImageModifications.back().subresourceRange;
	}

	VkImageMemoryBarrier transitionBarrier = { .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
											   .srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT,
											   .dstAccessMask = 0,
											   .oldLayout = lastSwapchainImageLayout,
											   .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
											   .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
											   .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
											   .image = m_gpuContext->swapchainImages()[result.imageIndex],
											   .subresourceRange = accessRange };

	vkCmdPipelineBarrier(frameCommandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
						 0, 0, nullptr, 0, nullptr, 1, &transitionBarrier);

	verifyResult(vkEndCommandBuffer(frameCommandBuffer));
	return frameCommandBuffer;
}

void VFramegraphContext::handleSwapchainResize(uint32_t width, uint32_t height) {
	for (auto& image : m_swapchainImageViews) {
		for (auto& viewInfo : image) {
			vkDestroyImageView(m_gpuContext->device(), viewInfo.second, nullptr);
		}
	}

	m_swapchainImageViews.resize(m_gpuContext->swapchainImages().size());

	for (size_t i = 0; i < m_swapchainImageViews.size(); ++i) {
		m_swapchainImageViews[i] = m_swapchainImageViews[0];

		if constexpr (vanadiumGPUDebug) {
			setObjectName(m_gpuContext->device(), VK_OBJECT_TYPE_IMAGE, m_gpuContext->swapchainImages()[i],
						  "Swapchain image for index " + std::to_string(i));
		}

		auto& imageViewMap = m_swapchainImageViews[i];
		for (auto& viewInfo : imageViewMap) {
			VkImageViewCreateInfo createInfo = { .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
												 .flags = viewInfo.first.flags,
												 .image = m_gpuContext->swapchainImages()[i],
												 .viewType = viewInfo.first.viewType,
												 .format = m_gpuContext->swapchainImageFormat(),
												 .components = viewInfo.first.components,
												 .subresourceRange = viewInfo.first.subresourceRange };
			vkCreateImageView(m_gpuContext->device(), &createInfo, nullptr, &viewInfo.second);

			if constexpr (vanadiumGPUDebug) {
				setObjectName(m_gpuContext->device(), VK_OBJECT_TYPE_IMAGE_VIEW, viewInfo.second,
							  "Swapchain image view for index " + std::to_string(i));
			}
		}
	}

	for (auto& node : m_nodes) {
		node.node->handleWindowResize(this, width, height);
	}
	updateDependencyInfo();
}

void VFramegraphContext::destroy() {

	for (auto& node : m_nodes) {
		node.node->destroy(this);
	}

	for (auto& views : m_swapchainImageViews) {
		for (auto& view : views) {
			vkDestroyImageView(m_gpuContext->device(), view.second, nullptr);
		}
	}
}

void VFramegraphContext::createBuffer(VFramegraphBufferHandle handle) {
	auto& parameters = m_buffers[handle].creationParameters;
	auto usage = m_buffers[handle].usageFlags;

	VkBufferCreateInfo createInfo = { .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
									  .flags = parameters.flags,
									  .size = parameters.size,
									  .usage = usage,
									  .sharingMode = VK_SHARING_MODE_EXCLUSIVE };
	m_buffers[handle].resourceHandle =
		m_resourceAllocator->createBuffer(createInfo, {}, { .deviceLocal = true }, false);
}

void VFramegraphContext::createImage(VFramegraphImageHandle handle) {
	auto& parameters = m_images[handle].creationParameters;
	auto usage = m_images[handle].usage;

	VkImageCreateInfo createInfo = { .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
									 .flags = parameters.flags,
									 .imageType = parameters.imageType,
									 .format = parameters.format,
									 .extent = parameters.extent,
									 .mipLevels = parameters.mipLevels,
									 .arrayLayers = parameters.arrayLayers,
									 .samples = parameters.samples,
									 .tiling = parameters.tiling,
									 .usage = usage,
									 .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED };
	m_images[handle].resourceHandle = m_resourceAllocator->createImage(createInfo, {}, { .deviceLocal = true });
}

void VFramegraphContext::addUsage(VFramegraphBufferHandle handle, size_t nodeIndex, std::vector<VFramegraphNodeBufferAccess>& modifications,
								  std::vector<VFramegraphNodeBufferAccess>& reads,
								  const VFramegraphNodeBufferUsage& usage) {
	m_buffers[handle].usageFlags |= usage.usageFlags;
	VFramegraphNodeBufferAccess access = {
		.usingNodeIndex = nodeIndex,
		.stageFlags = usage.pipelineStages,
		.accessFlags = usage.accessTypes,
		.offset = usage.offset,
		.size = usage.size,
	};

	if (usage.writes) {
		auto insertIterator = std::lower_bound(modifications.begin(), modifications.end(), access);
		reads.insert(insertIterator, access);
	} else {
		auto insertIterator = std::lower_bound(reads.begin(), reads.end(), access);
		reads.insert(insertIterator, access);
	}
}

void VFramegraphContext::addUsage(VFramegraphImageHandle handle, size_t nodeIndex,
								  std::vector<VFramegraphNodeImageAccess>& modifications,
								  std::vector<VFramegraphNodeImageAccess>& reads,
								  const VFramegraphNodeImageUsage& usage) {
	m_images[handle].usage |= usage.usageFlags;
	addUsage(nodeIndex, modifications, reads, usage);
}

void VFramegraphContext::addUsage(size_t nodeIndex, std::vector<VFramegraphNodeImageAccess>& modifications,
								  std::vector<VFramegraphNodeImageAccess>& reads,
								  const VFramegraphNodeImageUsage& usage) {
	VFramegraphNodeImageAccess access = { .usingNodeIndex = nodeIndex,
										  .stageFlags = usage.pipelineStages,
										  .accessFlags = usage.accessTypes,
										  .startLayout = usage.startLayout,
										  .finishLayout = usage.finishLayout,
										  .subresourceRange = usage.subresourceRange };

	if (usage.writes) {
		auto insertIterator = std::lower_bound(modifications.begin(), modifications.end(), access);
		modifications.insert(insertIterator, access);
	} else {
		auto insertIterator = std::lower_bound(reads.begin(), reads.end(), access);
		if (insertIterator != reads.end() && insertIterator->finishLayout != usage.startLayout) {
			insertIterator = std::lower_bound(modifications.begin(), modifications.end(), access);
			modifications.insert(insertIterator, access);
		} else {
			reads.insert(insertIterator, access);
		}
	}
}

void VFramegraphContext::updateDependencyInfo() {
	m_firstFrameFlag = true;
	m_nodeBufferMemoryBarriers.resize(m_nodes.size());
	m_nodeImageMemoryBarriers.resize(m_nodes.size());
	m_nodeBarrierStages.resize(m_nodes.size());
	m_initImageMemoryBarriers.clear();
	m_frameStartImageMemoryBarriers.clear();

	for (auto& buffer : m_buffers) {
		size_t firstUsageNodeIndex = ~0U;
		VFramegraphNodeBufferAccess firstUsageAccess;

		size_t lastUsageNodeIndex = 0U;
		VFramegraphNodeBufferAccess lastUsageAccess;

		for (auto& readWrite : buffer.modifications) {
			if (firstUsageNodeIndex > readWrite.usingNodeIndex) {
				firstUsageNodeIndex = readWrite.usingNodeIndex;
				firstUsageAccess = readWrite;
			}
			if (lastUsageNodeIndex <= readWrite.usingNodeIndex) {
				lastUsageNodeIndex = readWrite.usingNodeIndex;
				lastUsageAccess = readWrite;
			}

			emitBarriersForRead(m_resourceAllocator->nativeBufferHandle(buffer.resourceHandle), buffer.modifications,
								readWrite);
		}
		for (auto& read : buffer.reads) {
			if (firstUsageNodeIndex > read.usingNodeIndex) {
				firstUsageNodeIndex = read.usingNodeIndex;
				firstUsageAccess = read;
			}
			if (lastUsageNodeIndex <= read.usingNodeIndex) {
				lastUsageNodeIndex = read.usingNodeIndex;
				lastUsageAccess = read;
			}

			emitBarriersForRead(m_resourceAllocator->nativeBufferHandle(buffer.resourceHandle), buffer.modifications,
								read);
		}

		VkBufferMemoryBarrier usageSyncBarrier = { .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
												   .srcAccessMask = lastUsageAccess.accessFlags,
												   .dstAccessMask = firstUsageAccess.accessFlags,
												   .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
												   .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
												   .buffer =
													   m_resourceAllocator->nativeBufferHandle(buffer.resourceHandle),
												   .offset = firstUsageAccess.offset,
												   .size = firstUsageAccess.size };
		if (firstUsageNodeIndex != ~0U) {
			m_frameStartBufferMemoryBarriers.push_back(usageSyncBarrier);
			usageSyncBarrier.srcAccessMask = 0;

			m_frameStartBarrierStages.src |= lastUsageAccess.stageFlags;
			m_frameStartBarrierStages.dst |= firstUsageAccess.stageFlags;
		}
	}
	for (auto& image : m_images) {
		size_t firstUsageNodeIndex = ~0U;
		VFramegraphNodeImageAccess firstUsageAccess;

		size_t lastUsageNodeIndex = 0U;
		VFramegraphNodeImageAccess lastUsageAccess;
		for (auto& readWrite : image.modifications) {
			if (firstUsageNodeIndex > readWrite.usingNodeIndex) {
				firstUsageNodeIndex = readWrite.usingNodeIndex;
				firstUsageAccess = readWrite;
			}
			if (lastUsageNodeIndex <= readWrite.usingNodeIndex) {
				lastUsageNodeIndex = readWrite.usingNodeIndex;
				lastUsageAccess = readWrite;
			}

			emitBarriersForRead(m_resourceAllocator->nativeImageHandle(image.resourceHandle), image.modifications,
								readWrite);
		}
		for (auto& read : image.reads) {
			if (firstUsageNodeIndex > read.usingNodeIndex) {
				firstUsageNodeIndex = read.usingNodeIndex;
				firstUsageAccess = read;
			}
			if (lastUsageNodeIndex <= read.usingNodeIndex) {
				lastUsageNodeIndex = read.usingNodeIndex;
				lastUsageAccess = read;
			}

			emitBarriersForRead(m_resourceAllocator->nativeImageHandle(image.resourceHandle), image.modifications,
								read);
		}

		VkImageLayout syncDstLayout = firstUsageAccess.startLayout;
		if (syncDstLayout == VK_IMAGE_LAYOUT_UNDEFINED) {
			syncDstLayout = lastUsageAccess.finishLayout;
		}

		VkImageMemoryBarrier usageSyncBarrier = { .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
												  .srcAccessMask = lastUsageAccess.accessFlags,
												  .dstAccessMask = firstUsageAccess.accessFlags,
												  .oldLayout = lastUsageAccess.finishLayout,
												  .newLayout = firstUsageAccess.startLayout,
												  .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
												  .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
												  .image = m_resourceAllocator->nativeImageHandle(image.resourceHandle),
												  .subresourceRange = firstUsageAccess.subresourceRange };
		if (firstUsageNodeIndex != ~0U) {
			m_frameStartImageMemoryBarriers.push_back(usageSyncBarrier);
			usageSyncBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			usageSyncBarrier.srcAccessMask = 0;
			m_initImageMemoryBarriers.push_back(usageSyncBarrier);

			m_frameStartBarrierStages.src |= lastUsageAccess.stageFlags;
			m_frameStartBarrierStages.dst |= firstUsageAccess.stageFlags;
			m_initBarrierStages.src |= firstUsageAccess.stageFlags;
			m_initBarrierStages.dst |= firstUsageAccess.stageFlags;
		}
	}

	for (auto& readWrite : m_swapchainImageModifications) {
		emitBarriersForRead(VK_NULL_HANDLE, m_swapchainImageModifications, readWrite);
	}
	for (auto& read : m_swapchainImageReads) {
		emitBarriersForRead(VK_NULL_HANDLE, m_swapchainImageModifications, read);
	}
}

void VFramegraphContext::updateBarriers(uint32_t imageIndex) {
	m_frameStartImageMemoryBarriers.clear();
	m_frameStartBufferMemoryBarriers.clear();
	for (auto& barrierVector : m_nodeBufferMemoryBarriers) {
		barrierVector.clear();
	}
	for (auto& barrierVector : m_nodeImageMemoryBarriers) {
		barrierVector.clear();
	}
	for (auto& stages : m_nodeBarrierStages) {
		stages = {};
	}

	for (auto& buffer : m_buffers) {
		for (auto& modification : buffer.modifications) {
			if (!modification.barrier.has_value())
				continue;
			auto& barrier = modification.barrier.value();
			barrier.barrier.buffer = m_resourceAllocator->nativeBufferHandle(buffer.resourceHandle);
			m_nodeBufferMemoryBarriers[barrier.nodeIndex].push_back(barrier.barrier);
			m_nodeBarrierStages[barrier.nodeIndex].src |= barrier.srcStages;
			m_nodeBarrierStages[barrier.nodeIndex].dst |= barrier.dstStages;
		}
	}

	for (auto& image : m_images) {
		for (auto& modification : image.modifications) {
			if (!modification.barrier.has_value())
				continue;
			auto& barrier = modification.barrier.value();
			barrier.barrier.image = m_resourceAllocator->nativeImageHandle(image.resourceHandle);
			m_nodeImageMemoryBarriers[barrier.nodeIndex].push_back(barrier.barrier);
			m_nodeBarrierStages[barrier.nodeIndex].src |= barrier.srcStages;
			m_nodeBarrierStages[barrier.nodeIndex].dst |= barrier.dstStages;
		}
	}

	for (auto& modification : m_swapchainImageModifications) {
		if (!modification.barrier.has_value())
			continue;
		auto& barrier = modification.barrier.value();
		barrier.barrier.image = m_gpuContext->swapchainImages()[imageIndex];
		m_nodeImageMemoryBarriers[barrier.nodeIndex].push_back(barrier.barrier);
		m_nodeBarrierStages[barrier.nodeIndex].src |= barrier.srcStages;
		m_nodeBarrierStages[barrier.nodeIndex].dst |= barrier.dstStages;
	}

	std::optional<VFramegraphNodeImageAccess> firstSwapchainAccess;
	if (!m_swapchainImageReads.empty() && !m_swapchainImageModifications.empty()) {
		if (m_swapchainImageReads[0].usingNodeIndex < m_swapchainImageModifications[0].usingNodeIndex) {
			firstSwapchainAccess = m_swapchainImageReads[0];
		} else {
			firstSwapchainAccess = m_swapchainImageModifications[0];
		}
	} else if (m_swapchainImageModifications.empty()) {
		firstSwapchainAccess = m_swapchainImageReads[0];
	} else if (m_swapchainImageReads.empty()) {
		firstSwapchainAccess = m_swapchainImageModifications[0];
	}

	if (firstSwapchainAccess.has_value() && firstSwapchainAccess.value().startLayout != VK_IMAGE_LAYOUT_UNDEFINED) {
		VkImageMemoryBarrier barrier = { .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
										 .srcAccessMask = 0,
										 .dstAccessMask = firstSwapchainAccess.value().accessFlags,
										 .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
										 .newLayout = firstSwapchainAccess.value().startLayout,
										 .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
										 .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
										 .image = m_gpuContext->swapchainImages()[imageIndex],
										 .subresourceRange = firstSwapchainAccess.value().subresourceRange };
		m_frameStartImageMemoryBarriers.push_back(barrier);
		m_initImageMemoryBarriers.push_back(barrier);
		m_frameStartBarrierStages.src |= firstSwapchainAccess.value().stageFlags;
		m_frameStartBarrierStages.dst |= firstSwapchainAccess.value().stageFlags;
		m_initBarrierStages.src |= firstSwapchainAccess.value().stageFlags;
		m_initBarrierStages.dst |= firstSwapchainAccess.value().stageFlags;
	}
}

void VFramegraphContext::emitBarriersForRead(VkBuffer buffer, std::vector<VFramegraphNodeBufferAccess>& modifications,
											 const VFramegraphNodeBufferAccess& read) {
	auto matchOptional = findLastModification(modifications, read);
	if (!matchOptional.has_value())
		return;
	auto& match = matchOptional.value();

	if (match.isPartial) {
		if (match.offset > read.offset) {
			VFramegraphNodeBufferAccess partialRead = read;
			partialRead.offset = read.offset;
			partialRead.size = match.offset - read.offset;
			emitBarriersForRead(buffer, modifications, partialRead);
		}
		if (read.size == VK_WHOLE_SIZE && match.size != VK_WHOLE_SIZE) {
			VFramegraphNodeBufferAccess partialRead = read;
			partialRead.offset = match.offset + match.size;
			emitBarriersForRead(buffer, modifications, partialRead);
		} else if (match.size != VK_WHOLE_SIZE && match.offset + match.size < read.offset + read.size) {
			VFramegraphNodeBufferAccess partialRead = read;
			partialRead.offset = match.offset + match.size;
			partialRead.size = (read.offset + read.size) - (partialRead.offset);
			emitBarriersForRead(buffer, modifications, partialRead);
		}
	}

	emitBarrier(buffer, modifications[match.matchIndex], read, match);
}

void VFramegraphContext::emitBarriersForRead(VkImage image, std::vector<VFramegraphNodeImageAccess>& modifications,
											 const VFramegraphNodeImageAccess& read) {
	auto matchOptional = findLastModification(modifications, read);
	if (!matchOptional.has_value())
		return;
	auto& match = matchOptional.value();

	if (match.isPartial) {
		if (match.range.aspectMask != read.subresourceRange.aspectMask) {
			VFramegraphNodeImageAccess partialRead = read;
			partialRead.accessFlags = read.subresourceRange.aspectMask & ~match.range.aspectMask;
			emitBarriersForRead(image, modifications, partialRead);
		}

		if (match.range.baseArrayLayer > read.subresourceRange.baseArrayLayer) {
			VFramegraphNodeImageAccess partialRead = read;
			partialRead.subresourceRange.baseArrayLayer = read.subresourceRange.baseArrayLayer;
			partialRead.subresourceRange.layerCount = match.range.baseArrayLayer - read.subresourceRange.baseArrayLayer;
			emitBarriersForRead(image, modifications, partialRead);
		} else if (read.subresourceRange.layerCount == VK_REMAINING_ARRAY_LAYERS &&
				   match.range.layerCount != VK_REMAINING_ARRAY_LAYERS) {
			VFramegraphNodeImageAccess partialRead = read;
			partialRead.subresourceRange.baseArrayLayer = match.range.baseArrayLayer + match.range.layerCount;
			emitBarriersForRead(image, modifications, partialRead);
		} else if (match.range.layerCount != VK_REMAINING_ARRAY_LAYERS &&
				   match.range.baseArrayLayer + match.range.layerCount <
					   read.subresourceRange.baseArrayLayer + read.subresourceRange.layerCount) {
			VFramegraphNodeImageAccess partialRead = read;
			partialRead.subresourceRange.baseArrayLayer = match.range.baseArrayLayer + match.range.layerCount;
			partialRead.subresourceRange.layerCount =
				(read.subresourceRange.baseArrayLayer + read.subresourceRange.layerCount) -
				(partialRead.subresourceRange.baseArrayLayer);
			emitBarriersForRead(image, modifications, partialRead);
		}

		if (match.range.baseMipLevel > read.subresourceRange.baseMipLevel) {
			VFramegraphNodeImageAccess partialRead = read;
			partialRead.subresourceRange.baseMipLevel = read.subresourceRange.baseMipLevel;
			partialRead.subresourceRange.levelCount = match.range.baseMipLevel - read.subresourceRange.baseMipLevel;
			emitBarriersForRead(image, modifications, partialRead);
		} else if (read.subresourceRange.levelCount == VK_REMAINING_MIP_LEVELS &&
				   match.range.levelCount != VK_REMAINING_MIP_LEVELS) {
			VFramegraphNodeImageAccess partialRead = read;
			partialRead.subresourceRange.baseMipLevel = match.range.baseMipLevel + match.range.levelCount;
			emitBarriersForRead(image, modifications, partialRead);
		} else if (match.range.levelCount != VK_REMAINING_MIP_LEVELS &&
				   match.range.baseMipLevel + match.range.levelCount <
					   read.subresourceRange.baseMipLevel + read.subresourceRange.levelCount) {
			VFramegraphNodeImageAccess partialRead = read;
			partialRead.subresourceRange.baseMipLevel = match.range.baseMipLevel + match.range.levelCount;
			partialRead.subresourceRange.levelCount =
				(read.subresourceRange.baseMipLevel + read.subresourceRange.levelCount) -
				(partialRead.subresourceRange.baseMipLevel);
			emitBarriersForRead(image, modifications, partialRead);
		}
	}

	emitBarrier(image, modifications[match.matchIndex], read, match);
}

std::optional<VFramegraphBufferAccessMatch> VFramegraphContext::findLastModification(
	std::vector<VFramegraphNodeBufferAccess>& modifications, const VFramegraphNodeBufferAccess& read) {
	if (modifications.empty())
		return std::nullopt;
	auto nextModificationIterator = std::lower_bound(modifications.begin(), modifications.end(), read);

	if (nextModificationIterator == modifications.begin() || nextModificationIterator == modifications.end()) {
		return std::nullopt;
	}
	--nextModificationIterator;

	while (true) {
		if (overlaps<VkDeviceSize, VK_WHOLE_SIZE>(nextModificationIterator->offset, nextModificationIterator->size,
												  read.offset, read.size)) {
			// match was found
			VFramegraphBufferAccessMatch match = { .matchIndex = static_cast<size_t>(nextModificationIterator -
																					 modifications.begin()) };
			if (isCompletelyContained<VkDeviceSize, VK_WHOLE_SIZE>(
					nextModificationIterator->offset, nextModificationIterator->size, read.offset, read.size)) {
				match.isPartial = false;
				match.offset = read.offset;
				match.size = read.size;
				return match;
			} else {
				match.isPartial = true;
				match.offset = std::max(read.offset, nextModificationIterator->offset);
				if (read.size == VK_WHOLE_SIZE) {
					if (nextModificationIterator->size == VK_WHOLE_SIZE) {
						match.size = VK_WHOLE_SIZE;
					} else {
						match.size = nextModificationIterator->offset + nextModificationIterator->size - match.offset;
					}
				} else if (nextModificationIterator->size == VK_WHOLE_SIZE) {
					match.size = read.offset + read.size - match.offset;
				} else {
					match.size = std::min(read.offset + read.size,
										  nextModificationIterator->offset + nextModificationIterator->size) -
								 match.offset;
				}
				return match;
			}
		}
		if (nextModificationIterator == modifications.begin()) {
			break;
		}
		--nextModificationIterator;
	}
	return std::nullopt;
}

std::optional<VFramegraphImageAccessMatch> VFramegraphContext::findLastModification(
	std::vector<VFramegraphNodeImageAccess>& modifications, const VFramegraphNodeImageAccess& read) {
	if (modifications.empty())
		return std::nullopt;
	auto nextModificationIterator = std::lower_bound(modifications.begin(), modifications.end(), read);

	if (nextModificationIterator == modifications.begin() || nextModificationIterator == modifications.end()) {
		return std::nullopt;
	}
	--nextModificationIterator;
	while (true) {
		auto& srcRange = nextModificationIterator->subresourceRange;
		auto& dstRange = read.subresourceRange;
		bool resourceMatches = srcRange.aspectMask & dstRange.aspectMask;
		resourceMatches &= overlaps<uint32_t, VK_REMAINING_ARRAY_LAYERS>(srcRange.baseArrayLayer, srcRange.layerCount,
																		 dstRange.baseArrayLayer, dstRange.layerCount);
		resourceMatches &= overlaps<uint32_t, VK_REMAINING_MIP_LEVELS>(srcRange.baseMipLevel, srcRange.levelCount,
																	   dstRange.baseMipLevel, dstRange.levelCount);
		if (resourceMatches) {
			VFramegraphImageAccessMatch match = { .matchIndex = static_cast<size_t>(nextModificationIterator -
																					modifications.begin()) };
			bool matchesFully = srcRange.aspectMask == dstRange.aspectMask;
			matchesFully &= isCompletelyContained<uint32_t, VK_REMAINING_ARRAY_LAYERS>(
				srcRange.baseArrayLayer, srcRange.layerCount, dstRange.baseArrayLayer, dstRange.layerCount);
			matchesFully &= isCompletelyContained<uint32_t, VK_REMAINING_MIP_LEVELS>(
				srcRange.baseMipLevel, srcRange.levelCount, dstRange.baseMipLevel, dstRange.levelCount);
			if (matchesFully) {
				match.isPartial = false;
				match.range = srcRange;
				return match;
			} else {
				match.isPartial = true;
				match.range.aspectMask = srcRange.aspectMask & dstRange.aspectMask;
				match.range.baseArrayLayer = std::max(srcRange.baseArrayLayer, dstRange.baseArrayLayer);

				if (srcRange.layerCount == VK_REMAINING_ARRAY_LAYERS) {
					if (dstRange.layerCount == VK_REMAINING_ARRAY_LAYERS) {
						match.range.layerCount = VK_REMAINING_ARRAY_LAYERS;
					} else {
						match.range.layerCount =
							srcRange.baseArrayLayer + srcRange.layerCount - match.range.baseArrayLayer;
					}
				} else if (dstRange.layerCount == VK_REMAINING_ARRAY_LAYERS) {
					match.range.layerCount = srcRange.baseArrayLayer + srcRange.layerCount - match.range.baseArrayLayer;
				} else {
					match.range.layerCount = std::min(srcRange.baseArrayLayer + srcRange.layerCount,
													  dstRange.baseArrayLayer + dstRange.layerCount) -
											 match.range.baseArrayLayer;
				}

				match.range.baseMipLevel = std::max(srcRange.baseMipLevel, dstRange.baseMipLevel);

				if (srcRange.levelCount == VK_REMAINING_MIP_LEVELS) {
					if (dstRange.levelCount == VK_REMAINING_MIP_LEVELS) {
						match.range.levelCount = VK_REMAINING_MIP_LEVELS;
					} else {
						match.range.levelCount = srcRange.baseMipLevel + srcRange.levelCount - match.range.baseMipLevel;
					}
				} else if (dstRange.levelCount == VK_REMAINING_MIP_LEVELS) {
					match.range.levelCount = srcRange.baseMipLevel + srcRange.levelCount - match.range.baseMipLevel;
				} else {
					match.range.levelCount = std::min(srcRange.baseMipLevel + srcRange.levelCount,
													  dstRange.baseMipLevel + dstRange.levelCount) -
											 match.range.baseMipLevel;
				}
				return match;
			}
		}
		if (nextModificationIterator == modifications.begin()) {
			break;
		}
		--nextModificationIterator;
	}
	return std::nullopt;
}

void VFramegraphContext::emitBarrier(VkBuffer buffer, VFramegraphNodeBufferAccess& modification,
									 const VFramegraphNodeBufferAccess& read,
									 const VFramegraphBufferAccessMatch& match) {
	if (!modification.barrier.has_value()) {
		modification.barrier = VFramegraphBufferBarrier{ .nodeIndex = read.usingNodeIndex,
														 .srcStages = modification.stageFlags,
														 .dstStages = read.stageFlags,
														 .barrier = { .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
																	  .srcAccessMask = modification.accessFlags,
																	  .dstAccessMask = read.accessFlags,
																	  .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
																	  .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
																	  .buffer = buffer,
																	  .offset = match.offset,
																	  .size = match.size } };
	} else {
		auto& barrier = modification.barrier.value();
		barrier.nodeIndex = std::min(barrier.nodeIndex, read.usingNodeIndex);
		barrier.dstStages |= read.stageFlags;
		barrier.barrier.dstAccessMask |= read.accessFlags;
		if (barrier.barrier.offset > match.offset) {
			barrier.barrier.offset = match.offset;
			if (barrier.barrier.size != VK_WHOLE_SIZE)
				barrier.barrier.size += barrier.barrier.offset - match.offset;
		}
		barrier.barrier.size =
			std::max(barrier.barrier.offset + barrier.barrier.size, match.offset + match.size) - barrier.barrier.offset;
	}
}

void VFramegraphContext::emitBarrier(VkImage image, VFramegraphNodeImageAccess& modification,
									 const VFramegraphNodeImageAccess& read, const VFramegraphImageAccessMatch& match) {
	if (!modification.barrier.has_value()) {
		VkImageLayout newLayout = read.startLayout;
		if (newLayout == VK_IMAGE_LAYOUT_UNDEFINED) {
			newLayout = modification.finishLayout;
		}
		modification.barrier = VFramegraphImageBarrier{ .nodeIndex = read.usingNodeIndex,
														.srcStages = modification.stageFlags,
														.dstStages = read.stageFlags,
														.barrier = { .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
																	 .srcAccessMask = modification.accessFlags,
																	 .dstAccessMask = read.accessFlags,
																	 .oldLayout = modification.finishLayout,
																	 .newLayout = newLayout,
																	 .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
																	 .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
																	 .image = image,
																	 .subresourceRange = match.range } };
	} else {
		auto& barrier = modification.barrier.value();
		barrier.nodeIndex = std::min(barrier.nodeIndex, read.usingNodeIndex);
		barrier.dstStages |= read.stageFlags;
		barrier.barrier.dstAccessMask |= read.accessFlags;

		auto& dstRange = barrier.barrier.subresourceRange;
		dstRange.aspectMask |= match.range.aspectMask;

		if (dstRange.baseArrayLayer > match.range.baseArrayLayer) {
			dstRange.baseArrayLayer = match.range.baseArrayLayer;
			if (dstRange.layerCount != VK_REMAINING_ARRAY_LAYERS)
				dstRange.layerCount += dstRange.baseArrayLayer - match.range.baseArrayLayer;
		}
		dstRange.layerCount = std::max(dstRange.baseArrayLayer + dstRange.layerCount,
									   match.range.baseArrayLayer + match.range.layerCount) -
							  dstRange.baseArrayLayer;
		if (dstRange.baseMipLevel > match.range.baseMipLevel) {
			dstRange.baseMipLevel = match.range.baseMipLevel;
			if (dstRange.levelCount != VK_REMAINING_MIP_LEVELS)
				dstRange.levelCount += dstRange.baseMipLevel - match.range.baseMipLevel;
		}
		dstRange.levelCount =
			std::max(dstRange.baseMipLevel + dstRange.levelCount, match.range.baseMipLevel + match.range.levelCount) -
			dstRange.baseMipLevel;
	}
}
