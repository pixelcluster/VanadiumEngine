#include <Debug.hpp>
#include <cstdio>
#include <graphics/framegraph/FramegraphContext.hpp>
#include <graphics/framegraph/FramegraphNode.hpp>
#include <graphics/helper/DebugHelper.hpp>
#include <graphics/helper/ErrorHelper.hpp>
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

namespace vanadium::graphics {
	void FramegraphContext::create(const RenderContext& context) {
		m_context = context;

		VkCommandPoolCreateInfo poolCreateInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
												   .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
												   .queueFamilyIndex =
													   m_context.deviceContext->graphicsQueueFamilyIndex() };

		for (size_t i = 0; i < frameInFlightCount; ++i) {
			verifyResult(vkCreateCommandPool(m_context.deviceContext->device(), &poolCreateInfo, nullptr,
											 &m_frameCommandPools[i]));
			VkCommandBufferAllocateInfo allocateInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
														 .commandPool = m_frameCommandPools[i],
														 .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
														 .commandBufferCount = 1 };
			verifyResult(
				vkAllocateCommandBuffers(m_context.deviceContext->device(), &allocateInfo, &m_frameCommandBuffers[i]));
		}
	}

	void FramegraphContext::setupResources() {
		for (auto& node : m_nodes) {
			node.node->setupResources(this);
		}
	}

	void FramegraphContext::initResources() {
		for (auto handle : m_transientBuffers) {
			createBuffer(handle);
		}
		for (auto handle : m_transientImages) {
			createImage(handle);
		}

		for (auto& node : m_nodes) {
			for (auto& infos : node.resourceViewInfos) {
				for (auto& info : infos.second) {
					m_context.resourceAllocator->requestImageView(m_images[infos.first].resourceHandle, info);
				}
			}
			for(auto& info : node.swapchainResourceViewInfos) {
				m_context.targetSurface->addRequestedView(info);
			}
			node.node->initResources(this);
		}

		for (auto& node : m_nodes) {
			node.node->handleWindowResize(this, m_context.targetSurface->properties().width,
										  m_context.targetSurface->properties().height);
		}

		updateDependencyInfo();
	}

	FramegraphBufferHandle FramegraphContext::declareTransientBuffer(
		FramegraphNode* creator, const FramegraphBufferCreationParameters& parameters,
		const FramegraphNodeBufferUsage& usage) {
		FramegraphBufferHandle handle =
			m_buffers.addElement(FramegraphBufferResource{ .creationParameters = parameters });
		m_transientBuffers.push_back(handle);

		auto nodeIterator =
			std::find_if(m_nodes.begin(), m_nodes.end(), [creator](const auto& info) { return info.node == creator; });
		if (nodeIterator == m_nodes.end()) {
			printf("invalid node as creator!\n");
			return ~0U;
		}

		auto usageIterator = m_buffers.find(handle);
		if (usageIterator == m_buffers.end()) {
			printf("invalid resource for dependency!\n");
			return ~0U;
		}

		size_t nodeIndex = nodeIterator - m_nodes.begin();
		m_buffers[handle].usageFlags |= usage.usageFlags;
		m_barrierGenerator.addNodeBufferAccess(
			nodeIterator - m_nodes.begin(),
			NodeBufferAccess{ .subresourceAccesses = usage.subresourceAccesses, .buffer = handle });
		return handle;
	}

	FramegraphImageHandle FramegraphContext::declareTransientImage(FramegraphNode* creator,
																   const FramegraphImageCreationParameters& parameters,
																   const FramegraphNodeImageUsage& usage) {
		FramegraphImageHandle handle = m_images.addElement(FramegraphImageResource{ .creationParameters = parameters });
		m_transientImages.push_back(handle);

		auto nodeIterator =
			std::find_if(m_nodes.begin(), m_nodes.end(), [creator](const auto& info) { return info.node == creator; });
		if (nodeIterator == m_nodes.end()) {
			printf("invalid node as creator!\n");
			return ~0U;
		}
		size_t nodeIndex = nodeIterator - m_nodes.begin();

		m_images[handle].usage |= usage.usageFlags;
		m_barrierGenerator.addNodeImageAccess(
			nodeIterator - m_nodes.begin(),
			NodeImageAccess{ .subresourceAccesses = usage.subresourceAccesses, .image = handle });
		if (!usage.viewInfos.empty()) {
			nodeIterator->resourceViewInfos.insert(
				robin_hood::pair<FramegraphImageHandle, std::vector<ImageResourceViewInfo>>(handle, usage.viewInfos));
		}
		return handle;
	}

	FramegraphBufferHandle FramegraphContext::declareImportedBuffer(FramegraphNode* creator,
																	BufferResourceHandle handle,
																	const FramegraphNodeBufferUsage& usage) {

		FramegraphBufferHandle bufferHandle =
			m_buffers.addElement(FramegraphBufferResource{ .resourceHandle = handle });

		auto nodeIterator =
			std::find_if(m_nodes.begin(), m_nodes.end(), [creator](const auto& info) { return info.node == creator; });
		if (nodeIterator == m_nodes.end()) {
			printf("invalid node as creator!\n");
			return ~0U;
		}

		auto usageIterator = m_buffers.find(bufferHandle);
		if (usageIterator == m_buffers.end()) {
			printf("invalid resource for dependency!\n");
			return ~0U;
		}

		size_t nodeIndex = nodeIterator - m_nodes.begin();

		m_buffers[bufferHandle].usageFlags |= usage.usageFlags;
		m_barrierGenerator.addNodeBufferAccess(
			nodeIterator - m_nodes.begin(),
			NodeBufferAccess{ .subresourceAccesses = usage.subresourceAccesses, .buffer = handle });
		return bufferHandle;
	}

	FramegraphImageHandle FramegraphContext::declareImportedImage(FramegraphNode* creator, ImageResourceHandle handle,
																  const FramegraphNodeImageUsage& usage) {
		FramegraphImageHandle imageHandle = m_images.addElement(FramegraphImageResource{ .resourceHandle = handle });

		auto nodeIterator =
			std::find_if(m_nodes.begin(), m_nodes.end(), [creator](const auto& info) { return info.node == creator; });
		if (nodeIterator == m_nodes.end()) {
			printf("invalid node as creator!\n");
			return ~0U;
		}
		size_t nodeIndex = nodeIterator - m_nodes.begin();

		auto usageIterator = m_images.find(imageHandle);
		m_images[imageHandle].usage |= usage.usageFlags;
		m_barrierGenerator.addNodeImageAccess(
			nodeIterator - m_nodes.begin(),
			NodeImageAccess{ .subresourceAccesses = usage.subresourceAccesses, .image = handle });
		if (!usage.viewInfos.empty()) {
			nodeIterator->resourceViewInfos.insert(
				robin_hood::pair<FramegraphImageHandle, std::vector<ImageResourceViewInfo>>(imageHandle,
																							usage.viewInfos));
		}
		return imageHandle;
	}

	void FramegraphContext::declareReferencedBuffer(FramegraphNode* user, FramegraphBufferHandle handle,
													const FramegraphNodeBufferUsage& usage) {
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

		m_buffers[handle].usageFlags |= usage.usageFlags;
		m_barrierGenerator.addNodeBufferAccess(
			nodeIterator - m_nodes.begin(),
			NodeBufferAccess{ .subresourceAccesses = usage.subresourceAccesses, .buffer = handle });
	}

	void FramegraphContext::declareReferencedImage(FramegraphNode* user, FramegraphImageHandle handle,
												   const FramegraphNodeImageUsage& usage) {

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

		m_images[handle].usage |= usage.usageFlags;
		m_barrierGenerator.addNodeImageAccess(
			nodeIterator - m_nodes.begin(),
			NodeImageAccess{ .subresourceAccesses = usage.subresourceAccesses, .image = handle });
		if (!usage.viewInfos.empty()) {
			if (nodeIterator == m_nodes.end()) {
				printf("invalid node as creator!\n");
				return;
			}
			nodeIterator->resourceViewInfos.insert(
				robin_hood::pair<FramegraphImageHandle, std::vector<ImageResourceViewInfo>>(handle, usage.viewInfos));
		}
	}

	void FramegraphContext::declareReferencedSwapchainImage(FramegraphNode* user,
															const FramegraphNodeImageUsage& usage) {
		auto nodeIterator =
			std::find_if(m_nodes.begin(), m_nodes.end(), [user](const auto& info) { return info.node == user; });
		if (nodeIterator == m_nodes.end()) {
			printf("invalid node for dependency!\n");
			return;
		}

		size_t nodeIndex = nodeIterator - m_nodes.begin();

		m_targetImageUsageFlags |= usage.usageFlags;
		m_barrierGenerator.addNodeTargetImageAccess(
			nodeIterator - m_nodes.begin(), NodeImageAccess{ .subresourceAccesses = usage.subresourceAccesses });
		for (auto& info : usage.viewInfos) {
			if (nodeIterator == m_nodes.end()) {
				printf("invalid node as creator!\n");
				return;
			}
			nodeIterator->swapchainResourceViewInfos.push_back(info);
		}
	}

	void FramegraphContext::invalidateBuffer(FramegraphBufferHandle handle, BufferResourceHandle newHandle) {
		m_buffers[handle].resourceHandle = newHandle;
	}

	void FramegraphContext::invalidateImage(FramegraphImageHandle handle, ImageResourceHandle newHandle) {
		m_images[handle].resourceHandle = newHandle;
	}

	FramegraphBufferResource FramegraphContext::bufferResource(FramegraphBufferHandle handle) const {
		auto iterator = m_buffers.find(handle);
		if (iterator == m_buffers.cend())
			return {};
		else
			return *iterator;
	}

	FramegraphImageResource FramegraphContext::imageResource(FramegraphImageHandle handle) const {
		auto iterator = m_images.find(handle);
		if (iterator == m_images.cend())
			return {};
		else
			return *iterator;
	}

	void FramegraphContext::recreateBufferResource(FramegraphBufferHandle handle,
												   const FramegraphBufferCreationParameters& parameters) {
		if constexpr (vanadiumDebug) {
			if (std::find(m_transientBuffers.begin(), m_transientBuffers.end(), handle) == m_transientBuffers.end()) {
				printf("Trying to recreate non-created buffer!");
			}
		}
		m_buffers[handle].creationParameters = parameters;
		createBuffer(handle);
	}

	void FramegraphContext::recreateImageResource(FramegraphImageHandle handle,
												  const FramegraphImageCreationParameters& parameters) {
		if constexpr (vanadiumDebug) {
			if (std::find(m_transientImages.begin(), m_transientImages.end(), handle) == m_transientImages.end()) {
				printf("Trying to recreate non-created image!");
			}
		}
		m_images[handle].creationParameters = parameters;
		createImage(handle);
	}

	VkBuffer FramegraphContext::nativeBufferHandle(FramegraphBufferHandle handle) {
		if constexpr (vanadiumDebug) {
			if (m_buffers.find(handle) == m_buffers.end()) {
				printf("Trying to get handle of non-created buffer");
			}
		}
		return m_context.resourceAllocator->nativeBufferHandle(m_buffers[handle].resourceHandle);
	}

	VkImage FramegraphContext::nativeImageHandle(FramegraphImageHandle handle) {
		if constexpr (vanadiumDebug) {
			if (m_images.find(handle) == m_images.end()) {
				printf("Trying to get handle of non-created image!");
			}
		}
		return m_context.resourceAllocator->nativeImageHandle(m_images[handle].resourceHandle);
	}

	VkImageView FramegraphContext::imageView(FramegraphNode* node, FramegraphImageHandle handle, size_t index) {
		auto nodeIterator =
			std::find_if(m_nodes.begin(), m_nodes.end(), [node](const auto& info) { return info.node == node; });
		if (nodeIterator == m_nodes.end()) {
			printf("invalid node for dependency!\n");
			return VK_NULL_HANDLE;
		}

		size_t nodeIndex = nodeIterator - m_nodes.begin();
		return m_context.resourceAllocator->requestImageView(m_images[handle].resourceHandle,
															 nodeIterator->resourceViewInfos[handle][index]);
	}

	VkImageView FramegraphContext::targetImageView(FramegraphNode* node, uint32_t index) {
		auto nodeIterator =
			std::find_if(m_nodes.begin(), m_nodes.end(), [node](const auto& info) { return info.node == node; });
		if (nodeIterator == m_nodes.end()) {
			printf("getting image view of unknown node!\n");
			return VK_NULL_HANDLE;
		}

		if (nodeIterator->swapchainResourceViewInfos.empty()) {
			printf("getting swapchain image view of node that doesn't use swapchain images!\n");
			return VK_NULL_HANDLE;
		} else
			return m_context.targetSurface->currentTargetView(nodeIterator->swapchainResourceViewInfos[index]);
	}

	VkBufferUsageFlags FramegraphContext::bufferUsageFlags(FramegraphBufferHandle handle) {
		return m_buffers[handle].usageFlags;
	}

	VkImageUsageFlags FramegraphContext::imageUsageFlags(FramegraphImageHandle handle) {
		return m_images[handle].usage;
	}

	VkCommandBuffer FramegraphContext::recordFrame(uint32_t frameIndex) {
		updateBarriers();

		vkResetCommandPool(m_context.deviceContext->device(), m_frameCommandPools[frameIndex], 0);

		VkCommandBuffer frameCommandBuffer = m_frameCommandBuffers[frameIndex];
		VkCommandBufferBeginInfo beginInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
											   .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT };
		verifyResult(vkBeginCommandBuffer(frameCommandBuffer, &beginInfo));

		size_t nodeIndex = 0;

		FramegraphNodeContext nodeContext = { .frameIndex = frameIndex, .targetSurface = m_context.targetSurface };

		if (m_barrierGenerator.frameStartBarrierCount()) {
			VkMemoryBarrier memoryBarrier = { .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
											  .srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
											  .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT };

			vkCmdPipelineBarrier(frameCommandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
								 VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 1, &memoryBarrier, 0, nullptr,
								 static_cast<uint32_t>(m_barrierGenerator.frameStartBarrierCount()),
								 m_barrierGenerator.frameStartBarriers().data());
		}

		for (auto& node : m_nodes) {
			if constexpr (vanadiumGPUDebug) {
				VkDebugUtilsLabelEXT label = { .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
											   .pLabelName = node.node->name().c_str() };
				vkCmdBeginDebugUtilsLabelEXT(frameCommandBuffer, &label);
			}

			nodeContext.resourceImageViews.clear();
			nodeContext.targetImageViews.clear();
			for (auto& viewInfos : node.resourceViewInfos) {
				std::vector<VkImageView> views;
				views.reserve(viewInfos.second.size());
				for (auto& info : viewInfos.second) {
					views.push_back(
						m_context.resourceAllocator->requestImageView(m_images[viewInfos.first].resourceHandle, info));
				}
				nodeContext.resourceImageViews.insert(
					robin_hood::pair<FramegraphImageHandle, std::vector<VkImageView>>(viewInfos.first, views));
			}

			if (!node.swapchainResourceViewInfos.empty()) {
				for (auto& info : node.swapchainResourceViewInfos)
					nodeContext.targetImageViews.push_back(m_context.targetSurface->currentTargetView(info));
			}

			node.node->recordCommands(this, frameCommandBuffer, nodeContext);

			if (m_barrierGenerator.bufferBarrierCount(nodeIndex) || m_barrierGenerator.imageBarrierCount(nodeIndex)) {
				vkCmdPipelineBarrier(frameCommandBuffer, m_barrierGenerator.srcStages(nodeIndex),
									 m_barrierGenerator.dstStages(nodeIndex), 0, 0, nullptr,
									 static_cast<uint32_t>(m_barrierGenerator.bufferBarrierCount(nodeIndex)),
									 m_barrierGenerator.bufferBarriers(nodeIndex).data(),
									 static_cast<uint32_t>(m_barrierGenerator.imageBarrierCount(nodeIndex)),
									 m_barrierGenerator.imageBarriers(nodeIndex).data());
			}

			if constexpr (vanadiumGPUDebug) {
				vkCmdEndDebugUtilsLabelEXT(frameCommandBuffer);
			}

			nodeContext.resourceImageViews.clear();
			++nodeIndex;
		}

		VkImageLayout lastSwapchainImageLayout = m_barrierGenerator.lastTargetImageLayout();
		VkImageSubresourceRange accessRange = m_barrierGenerator.lastTargetAccessRange();

		VkImageMemoryBarrier transitionBarrier = { .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
												   .srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT,
												   .dstAccessMask = 0,
												   .oldLayout = lastSwapchainImageLayout,
												   .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
												   .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
												   .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
												   .image = m_context.targetSurface->currentTargetImage(),
												   .subresourceRange = accessRange };

		vkCmdPipelineBarrier(frameCommandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
							 VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &transitionBarrier);

		verifyResult(vkEndCommandBuffer(frameCommandBuffer));
		return frameCommandBuffer;
	}

	void FramegraphContext::handleSwapchainResize(uint32_t width, uint32_t height) {
		for (auto& node : m_nodes) {
			node.node->handleWindowResize(this, width, height);
		}
		updateDependencyInfo();
	}

	void FramegraphContext::destroy() {
		for (auto& commandPool : m_frameCommandPools) {
			vkDestroyCommandPool(m_context.deviceContext->device(), commandPool, nullptr);
		}
		for (auto& node : m_nodes) {
			node.node->destroy(this);
			delete node.node;
		}
		m_nodes.clear();
	}

	void FramegraphContext::createBuffer(FramegraphBufferHandle handle) {
		auto& parameters = m_buffers[handle].creationParameters;
		auto usage = m_buffers[handle].usageFlags;

		VkBufferCreateInfo createInfo = { .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
										  .flags = parameters.flags,
										  .size = parameters.size,
										  .usage = usage,
										  .sharingMode = VK_SHARING_MODE_EXCLUSIVE };
		m_buffers[handle].resourceHandle =
			m_context.resourceAllocator->createBuffer(createInfo, {}, { .deviceLocal = true }, false);
	}

	void FramegraphContext::createImage(FramegraphImageHandle handle) {
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
		m_images[handle].resourceHandle =
			m_context.resourceAllocator->createImage(createInfo, {}, { .deviceLocal = true });
	}

	void FramegraphContext::updateDependencyInfo() {
		m_firstFrameFlag = true;
		m_barrierGenerator.create(m_nodes.size());
		m_barrierGenerator.generateDependencyInfo();
	}

	void FramegraphContext::updateBarriers() {
		m_barrierGenerator.generateBarrierInfo(m_firstFrameFlag, &FramegraphContext::nativeBufferHandle,
											   &FramegraphContext::nativeImageHandle, this,
											   m_context.targetSurface->currentTargetImage());
	}
} // namespace vanadium::graphics