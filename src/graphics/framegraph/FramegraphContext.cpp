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
	void FramegraphContext::create(DeviceContext* context, GPUResourceAllocator* resourceAllocator,
								   GPUDescriptorSetAllocator* descriptorSetAllocator,
								   GPUTransferManager* transferManager, RenderTargetSurface* targetSurface) {
		m_gpuContext = context;
		m_resourceAllocator = resourceAllocator;
		m_descriptorSetAllocator = descriptorSetAllocator;
		m_transferManager = transferManager;
		m_targetSurface = targetSurface;

		VkCommandPoolCreateInfo poolCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
			.queueFamilyIndex = m_gpuContext->graphicsQueueFamilyIndex()
		};

		for (size_t i = 0; i < frameInFlightCount; ++i) {
			verifyResult(vkCreateCommandPool(m_gpuContext->device(), &poolCreateInfo, nullptr, &m_frameCommandPools[i]));
			VkCommandBufferAllocateInfo allocateInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
														 .commandPool = m_frameCommandPools[i],
														 .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
														 .commandBufferCount = 1 };
			verifyResult(vkAllocateCommandBuffers(m_gpuContext->device(), &allocateInfo, &m_frameCommandBuffers[i]));
		}
	}

	void FramegraphContext::setupResources() {
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

	FramegraphImageHandle FramegraphContext::declareTransientImage(FramegraphNode* creator,
																   const FramegraphImageCreationParameters& parameters,
																   const FramegraphNodeImageUsage& usage) {
		FramegraphImageHandle handle = m_images.addElement(FramegraphImageResource{ .creationParameters = parameters });
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
				robin_hood::pair<const FramegraphImageHandle, ImageResourceViewInfo>(handle, usage.viewInfo.value()));
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

	FramegraphImageHandle FramegraphContext::declareImportedImage(FramegraphNode* creator, ImageResourceHandle handle,
																  const FramegraphNodeImageUsage& usage) {
		FramegraphImageHandle imageHandle = m_images.addElement(FramegraphImageResource{ .resourceHandle = handle });

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
				robin_hood::pair<const FramegraphImageHandle, ImageResourceViewInfo>(imageHandle, usage.viewInfo.value()));
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

		addUsage(handle, nodeIndex, usageIterator->modifications, usageIterator->reads, usage);
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

		addUsage(handle, nodeIndex, usageIterator->modifications, usageIterator->reads, usage);

		if (usage.viewInfo.has_value()) {
			if (nodeIterator == m_nodes.end()) {
				printf("invalid node as creator!\n");
				return;
			}
			nodeIterator->resourceViewInfos.insert(
				robin_hood::pair<const FramegraphImageHandle, ImageResourceViewInfo>(handle, usage.viewInfo.value()));
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

		addUsage(nodeIndex, m_swapchainImageModifications, m_swapchainImageReads, usage);
		m_swapchainImageUsageFlags |= usage.usageFlags;

		if (usage.viewInfo.has_value()) {
			if (nodeIterator == m_nodes.end()) {
				printf("invalid node as creator!\n");
				return;
			}
			nodeIterator->swapchainResourceViewInfo = usage.viewInfo.value();
			m_targetSurface->addRequestedView(usage.viewInfo.value());
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
		return m_resourceAllocator->nativeBufferHandle(m_buffers[handle].resourceHandle);
	}

	VkImage FramegraphContext::nativeImageHandle(FramegraphImageHandle handle) {
		if constexpr (vanadiumDebug) {
			if (m_images.find(handle) == m_images.end()) {
				printf("Trying to get handle of non-created image!");
			}
		}
		return m_resourceAllocator->nativeImageHandle(m_images[handle].resourceHandle);
	}

	VkImageView FramegraphContext::imageView(FramegraphNode* node, FramegraphImageHandle handle) {
		auto nodeIterator =
			std::find_if(m_nodes.begin(), m_nodes.end(), [node](const auto& info) { return info.node == node; });
		if (nodeIterator == m_nodes.end()) {
			printf("invalid node for dependency!\n");
			return VK_NULL_HANDLE;
		}

		size_t nodeIndex = nodeIterator - m_nodes.begin();
		return m_resourceAllocator->requestImageView(m_images[handle].resourceHandle,
													 nodeIterator->resourceViewInfos[handle]);
	}

	VkImageView FramegraphContext::targetImageView(FramegraphNode* node, uint32_t index) {
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
			return m_targetSurface->currentTargetView(nodeIterator->swapchainResourceViewInfo.value());
	}

	VkBufferUsageFlags FramegraphContext::bufferUsageFlags(FramegraphBufferHandle handle) {
		return m_buffers[handle].usageFlags;
	}

	VkImageUsageFlags FramegraphContext::imageUsageFlags(FramegraphImageHandle handle) {
		return m_images[handle].usage;
	}

	VkCommandBuffer FramegraphContext::recordFrame(uint32_t frameIndex) {
		updateBarriers();

		VkCommandBuffer frameCommandBuffer = m_frameCommandBuffers[frameIndex];

		VkCommandBufferBeginInfo beginInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
											   .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT };
		verifyResult(vkBeginCommandBuffer(frameCommandBuffer, &beginInfo));

		size_t nodeIndex = 0;

		FramegraphNodeContext nodeContext = { .frameIndex = frameIndex,
											  .targetSurface = m_targetSurface };

		if (m_firstFrameFlag) {
			if (!m_initImageMemoryBarriers.empty()) {
				vkCmdPipelineBarrier(frameCommandBuffer, m_initBarrierStages.src, m_initBarrierStages.dst, 0, 0,
									 nullptr, 0, nullptr, static_cast<uint32_t>(m_initImageMemoryBarriers.size()),
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
				nodeContext.resourceImageViews.insert(
					robin_hood::pair<FramegraphImageHandle, VkImageView>(viewInfo.first, view));
			}

			if (node.swapchainResourceViewInfo.has_value()) {
				nodeContext.targetImageView =
					m_targetSurface->currentTargetView(node.swapchainResourceViewInfo.value());
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
												   .image = m_targetSurface->currentTargetImage(),
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
		for (auto& node : m_nodes) {
			node.node->destroy(this);
		}
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
			m_resourceAllocator->createBuffer(createInfo, {}, { .deviceLocal = true }, false);
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
		m_images[handle].resourceHandle = m_resourceAllocator->createImage(createInfo, {}, { .deviceLocal = true });
	}

	void FramegraphContext::addUsage(FramegraphBufferHandle handle, size_t nodeIndex,
									 std::vector<FramegraphNodeBufferAccess>& modifications,
									 std::vector<FramegraphNodeBufferAccess>& reads,
									 const FramegraphNodeBufferUsage& usage) {
		m_buffers[handle].usageFlags |= usage.usageFlags;
		FramegraphNodeBufferAccess access = {
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

	void FramegraphContext::addUsage(FramegraphImageHandle handle, size_t nodeIndex,
									 std::vector<FramegraphNodeImageAccess>& modifications,
									 std::vector<FramegraphNodeImageAccess>& reads,
									 const FramegraphNodeImageUsage& usage) {
		m_images[handle].usage |= usage.usageFlags;
		addUsage(nodeIndex, modifications, reads, usage);
	}

	void FramegraphContext::addUsage(size_t nodeIndex, std::vector<FramegraphNodeImageAccess>& modifications,
									 std::vector<FramegraphNodeImageAccess>& reads,
									 const FramegraphNodeImageUsage& usage) {
		FramegraphNodeImageAccess access = { .usingNodeIndex = nodeIndex,
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

	void FramegraphContext::updateDependencyInfo() {
		m_firstFrameFlag = true;
		m_nodeBufferMemoryBarriers.resize(m_nodes.size());
		m_nodeImageMemoryBarriers.resize(m_nodes.size());
		m_nodeBarrierStages.resize(m_nodes.size());
		m_initImageMemoryBarriers.clear();
		m_frameStartImageMemoryBarriers.clear();

		for (auto& buffer : m_buffers) {
			size_t firstUsageNodeIndex = ~0U;
			FramegraphNodeBufferAccess firstUsageAccess;

			size_t lastUsageNodeIndex = 0U;
			FramegraphNodeBufferAccess lastUsageAccess;

			for (auto& readWrite : buffer.modifications) {
				if (firstUsageNodeIndex > readWrite.usingNodeIndex) {
					firstUsageNodeIndex = readWrite.usingNodeIndex;
					firstUsageAccess = readWrite;
				}
				if (lastUsageNodeIndex <= readWrite.usingNodeIndex) {
					lastUsageNodeIndex = readWrite.usingNodeIndex;
					lastUsageAccess = readWrite;
				}

				emitBarriersForRead(m_resourceAllocator->nativeBufferHandle(buffer.resourceHandle),
									buffer.modifications, readWrite);
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

				emitBarriersForRead(m_resourceAllocator->nativeBufferHandle(buffer.resourceHandle),
									buffer.modifications, read);
			}

			VkBufferMemoryBarrier usageSyncBarrier = { .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
													   .srcAccessMask = lastUsageAccess.accessFlags,
													   .dstAccessMask = firstUsageAccess.accessFlags,
													   .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
													   .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
													   .buffer = m_resourceAllocator->nativeBufferHandle(
														   buffer.resourceHandle),
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
			FramegraphNodeImageAccess firstUsageAccess;

			size_t lastUsageNodeIndex = 0U;
			FramegraphNodeImageAccess lastUsageAccess;
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
													  .image =
														  m_resourceAllocator->nativeImageHandle(image.resourceHandle),
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

	void FramegraphContext::updateBarriers() {
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
			barrier.barrier.image = m_targetSurface->currentTargetImage();
			m_nodeImageMemoryBarriers[barrier.nodeIndex].push_back(barrier.barrier);
			m_nodeBarrierStages[barrier.nodeIndex].src |= barrier.srcStages;
			m_nodeBarrierStages[barrier.nodeIndex].dst |= barrier.dstStages;
		}

		std::optional<FramegraphNodeImageAccess> firstSwapchainAccess;
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
											 .image = m_targetSurface->currentTargetImage(),
											 .subresourceRange = firstSwapchainAccess.value().subresourceRange };
			m_frameStartImageMemoryBarriers.push_back(barrier);
			m_initImageMemoryBarriers.push_back(barrier);
			m_frameStartBarrierStages.src |= firstSwapchainAccess.value().stageFlags;
			m_frameStartBarrierStages.dst |= firstSwapchainAccess.value().stageFlags;
			m_initBarrierStages.src |= firstSwapchainAccess.value().stageFlags;
			m_initBarrierStages.dst |= firstSwapchainAccess.value().stageFlags;
		}
	}

	void FramegraphContext::emitBarriersForRead(VkBuffer buffer, std::vector<FramegraphNodeBufferAccess>& modifications,
												const FramegraphNodeBufferAccess& read) {
		auto matchOptional = findLastModification(modifications, read);
		if (!matchOptional.has_value())
			return;
		auto& match = matchOptional.value();

		if (match.isPartial) {
			if (match.offset > read.offset) {
				FramegraphNodeBufferAccess partialRead = read;
				partialRead.offset = read.offset;
				partialRead.size = match.offset - read.offset;
				emitBarriersForRead(buffer, modifications, partialRead);
			}
			if (read.size == VK_WHOLE_SIZE && match.size != VK_WHOLE_SIZE) {
				FramegraphNodeBufferAccess partialRead = read;
				partialRead.offset = match.offset + match.size;
				emitBarriersForRead(buffer, modifications, partialRead);
			} else if (match.size != VK_WHOLE_SIZE && match.offset + match.size < read.offset + read.size) {
				FramegraphNodeBufferAccess partialRead = read;
				partialRead.offset = match.offset + match.size;
				partialRead.size = (read.offset + read.size) - (partialRead.offset);
				emitBarriersForRead(buffer, modifications, partialRead);
			}
		}

		emitBarrier(buffer, modifications[match.matchIndex], read, match);
	}

	void FramegraphContext::emitBarriersForRead(VkImage image, std::vector<FramegraphNodeImageAccess>& modifications,
												const FramegraphNodeImageAccess& read) {
		auto matchOptional = findLastModification(modifications, read);
		if (!matchOptional.has_value())
			return;
		auto& match = matchOptional.value();

		if (match.isPartial) {
			if (match.range.aspectMask != read.subresourceRange.aspectMask) {
				FramegraphNodeImageAccess partialRead = read;
				partialRead.accessFlags = read.subresourceRange.aspectMask & ~match.range.aspectMask;
				emitBarriersForRead(image, modifications, partialRead);
			}

			if (match.range.baseArrayLayer > read.subresourceRange.baseArrayLayer) {
				FramegraphNodeImageAccess partialRead = read;
				partialRead.subresourceRange.baseArrayLayer = read.subresourceRange.baseArrayLayer;
				partialRead.subresourceRange.layerCount =
					match.range.baseArrayLayer - read.subresourceRange.baseArrayLayer;
				emitBarriersForRead(image, modifications, partialRead);
			} else if (read.subresourceRange.layerCount == VK_REMAINING_ARRAY_LAYERS &&
					   match.range.layerCount != VK_REMAINING_ARRAY_LAYERS) {
				FramegraphNodeImageAccess partialRead = read;
				partialRead.subresourceRange.baseArrayLayer = match.range.baseArrayLayer + match.range.layerCount;
				emitBarriersForRead(image, modifications, partialRead);
			} else if (match.range.layerCount != VK_REMAINING_ARRAY_LAYERS &&
					   match.range.baseArrayLayer + match.range.layerCount <
						   read.subresourceRange.baseArrayLayer + read.subresourceRange.layerCount) {
				FramegraphNodeImageAccess partialRead = read;
				partialRead.subresourceRange.baseArrayLayer = match.range.baseArrayLayer + match.range.layerCount;
				partialRead.subresourceRange.layerCount =
					(read.subresourceRange.baseArrayLayer + read.subresourceRange.layerCount) -
					(partialRead.subresourceRange.baseArrayLayer);
				emitBarriersForRead(image, modifications, partialRead);
			}

			if (match.range.baseMipLevel > read.subresourceRange.baseMipLevel) {
				FramegraphNodeImageAccess partialRead = read;
				partialRead.subresourceRange.baseMipLevel = read.subresourceRange.baseMipLevel;
				partialRead.subresourceRange.levelCount = match.range.baseMipLevel - read.subresourceRange.baseMipLevel;
				emitBarriersForRead(image, modifications, partialRead);
			} else if (read.subresourceRange.levelCount == VK_REMAINING_MIP_LEVELS &&
					   match.range.levelCount != VK_REMAINING_MIP_LEVELS) {
				FramegraphNodeImageAccess partialRead = read;
				partialRead.subresourceRange.baseMipLevel = match.range.baseMipLevel + match.range.levelCount;
				emitBarriersForRead(image, modifications, partialRead);
			} else if (match.range.levelCount != VK_REMAINING_MIP_LEVELS &&
					   match.range.baseMipLevel + match.range.levelCount <
						   read.subresourceRange.baseMipLevel + read.subresourceRange.levelCount) {
				FramegraphNodeImageAccess partialRead = read;
				partialRead.subresourceRange.baseMipLevel = match.range.baseMipLevel + match.range.levelCount;
				partialRead.subresourceRange.levelCount =
					(read.subresourceRange.baseMipLevel + read.subresourceRange.levelCount) -
					(partialRead.subresourceRange.baseMipLevel);
				emitBarriersForRead(image, modifications, partialRead);
			}
		}

		emitBarrier(image, modifications[match.matchIndex], read, match);
	}

	std::optional<FramegraphBufferAccessMatch> FramegraphContext::findLastModification(
		std::vector<FramegraphNodeBufferAccess>& modifications, const FramegraphNodeBufferAccess& read) {
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
				FramegraphBufferAccessMatch match = { .matchIndex = static_cast<size_t>(nextModificationIterator -
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
							match.size =
								nextModificationIterator->offset + nextModificationIterator->size - match.offset;
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

	std::optional<FramegraphImageAccessMatch> FramegraphContext::findLastModification(
		std::vector<FramegraphNodeImageAccess>& modifications, const FramegraphNodeImageAccess& read) {
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
			resourceMatches &= overlaps<uint32_t, VK_REMAINING_ARRAY_LAYERS>(
				srcRange.baseArrayLayer, srcRange.layerCount, dstRange.baseArrayLayer, dstRange.layerCount);
			resourceMatches &= overlaps<uint32_t, VK_REMAINING_MIP_LEVELS>(srcRange.baseMipLevel, srcRange.levelCount,
																		   dstRange.baseMipLevel, dstRange.levelCount);
			if (resourceMatches) {
				FramegraphImageAccessMatch match = { .matchIndex = static_cast<size_t>(nextModificationIterator -
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
						match.range.layerCount =
							srcRange.baseArrayLayer + srcRange.layerCount - match.range.baseArrayLayer;
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
							match.range.levelCount =
								srcRange.baseMipLevel + srcRange.levelCount - match.range.baseMipLevel;
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

	void FramegraphContext::emitBarrier(VkBuffer buffer, FramegraphNodeBufferAccess& modification,
										const FramegraphNodeBufferAccess& read,
										const FramegraphBufferAccessMatch& match) {
		if (!modification.barrier.has_value()) {
			modification.barrier =
				FramegraphBufferBarrier{ .nodeIndex = read.usingNodeIndex,
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
			barrier.barrier.size = std::max(barrier.barrier.offset + barrier.barrier.size, match.offset + match.size) -
								   barrier.barrier.offset;
		}
	}

	void FramegraphContext::emitBarrier(VkImage image, FramegraphNodeImageAccess& modification,
										const FramegraphNodeImageAccess& read,
										const FramegraphImageAccessMatch& match) {
		if (!modification.barrier.has_value()) {
			VkImageLayout newLayout = read.startLayout;
			if (newLayout == VK_IMAGE_LAYOUT_UNDEFINED) {
				newLayout = modification.finishLayout;
			}
			modification.barrier = FramegraphImageBarrier{ .nodeIndex = read.usingNodeIndex,
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
			dstRange.levelCount = std::max(dstRange.baseMipLevel + dstRange.levelCount,
										   match.range.baseMipLevel + match.range.levelCount) -
								  dstRange.baseMipLevel;
		}
	}

} // namespace vanadium::graphics