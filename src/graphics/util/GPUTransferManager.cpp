#include <cstring>
#include <graphics/helper/ErrorHelper.hpp>
#include <graphics/util/GPUTransferManager.hpp>
#include <helper/SharedLockGuard.hpp>
#include <volk.h>

namespace vanadium::graphics {

	void GPUTransferManager::create(DeviceContext* context, GPUResourceAllocator* allocator) {
		m_context = context;
		m_resourceAllocator = allocator;
		m_stagingBufferAllocationFreeList.resize(frameInFlightCount);

		VkCommandPoolCreateInfo poolCreateInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
												   .queueFamilyIndex = m_context->graphicsQueueFamilyIndex() };

		for (size_t i = 0; i < frameInFlightCount; ++i) {
			verifyResult(
				vkCreateCommandPool(m_context->device(), &poolCreateInfo, nullptr, &m_transferCommandPools[i]));

			VkCommandBufferAllocateInfo allocateInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
														 .commandPool = m_transferCommandPools[i],
														 .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
														 .commandBufferCount = 1 };
			verifyResult(vkAllocateCommandBuffers(m_context->device(), &allocateInfo, &m_transferCommandBuffers[i]));
		}
	}

	GPUTransferHandle GPUTransferManager::createTransfer(VkDeviceSize transferBufferSize, VkBufferUsageFlags usageFlags,
														 VkPipelineStageFlags usageStageFlags,
														 VkAccessFlags usageAccessFlags) {
		auto lock = std::lock_guard<std::shared_mutex>(m_accessMutex);
		VkBufferCreateInfo transferBufferCreateInfo = { .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
														.size = transferBufferSize,
														.usage = usageFlags | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
														.sharingMode = VK_SHARING_MODE_EXCLUSIVE };
		BufferResourceHandle dstBuffer = m_resourceAllocator->createBuffer(
			transferBufferCreateInfo, {}, { .deviceLocal = true, .hostVisible = true }, true);
		std::vector<GPUTransfer> transfers;
		transfers.reserve(frameInFlightCount);
		for (size_t i = 0; i < frameInFlightCount; ++i) {
			GPUTransfer transfer = { .dstBuffer = dstBuffer,
									 .bufferSize = transferBufferSize,
									 .dstUsageStageFlags = usageStageFlags,
									 .dstUsageAccessFlags = usageAccessFlags };

			if (!m_resourceAllocator->bufferMemoryCapabilities(transfer.dstBuffer).hostVisible) {
				transfer.needsStagingBuffer = true;
				transfer.stagingBuffer = allocateStagingBufferArea(transferBufferSize);
			}
			transfers.push_back(transfer);
		}

		return m_continuousTransfers.addElement(transfers);
	}

	void GPUTransferManager::destroyTransfer(GPUTransferHandle handle) {
		for (auto& transfer : m_continuousTransfers[handle]) {
			if (transfer.needsStagingBuffer) {
				auto bufferHandle = transfer.stagingBuffer.bufferHandle;
				auto& allocationRange = transfer.stagingBuffer.allocationResult.allocationRange;
				freeToRanges(m_stagingBuffers[bufferHandle].freeRangesOffsetSorted,
							 m_stagingBuffers[bufferHandle].freeRangesSizeSorted, allocationRange.offset,
							 allocationRange.size);
			}
		}
		m_resourceAllocator->destroyBuffer(m_continuousTransfers[handle].back().dstBuffer);
		m_continuousTransfers.removeElement(handle);
	}

	AsyncBufferTransferHandle GPUTransferManager::createAsyncBufferTransfer(void* data, size_t size,
																			BufferResourceHandle dstBuffer,
																			size_t offset,
																			VkPipelineStageFlags usageStageFlags,
																			VkAccessFlags usageAccessFlags) {
		auto lock = std::lock_guard<std::shared_mutex>(m_accessMutex);
		AsyncBufferTransfer transfer = {
			.stagingBufferAllocation = allocateStagingBufferArea(size),
			.dstBufferHandle = dstBuffer,
			.copy = { .srcOffset = transfer.stagingBufferAllocation.allocationResult.usableRange.offset,
					  .dstOffset = offset,
					  .size = size },
			.transferBarrier = { .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
								 .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
								 .dstAccessMask = 0,
								 .srcQueueFamilyIndex = m_context->asyncTransferQueueFamilyIndex(),
								 .dstQueueFamilyIndex = m_context->graphicsQueueFamilyIndex(),
								 .buffer = m_resourceAllocator->nativeBufferHandle(dstBuffer),
								 .offset = offset,
								 .size = size },

			.acquireBarrier = { .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
								.srcAccessMask = 0,
								.dstAccessMask = usageAccessFlags,
								.srcQueueFamilyIndex = m_context->asyncTransferQueueFamilyIndex(),
								.dstQueueFamilyIndex = m_context->graphicsQueueFamilyIndex(),
								.buffer = m_resourceAllocator->nativeBufferHandle(dstBuffer),
								.offset = offset,
								.size = size },
			.dstStageFlags = usageStageFlags
		};
		std::memcpy(
			reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(m_resourceAllocator->mappedBufferData(
										m_stagingBuffers[transfer.stagingBufferAllocation.bufferHandle].buffer)) +
									transfer.stagingBufferAllocation.allocationResult.usableRange.offset),
			data, size);

		return m_asyncBufferTransfers.addElement(transfer);
	}

	AsyncImageTransferHandle GPUTransferManager::createAsyncImageTransfer(
		void* data, size_t size, ImageResourceHandle dstImage, const VkBufferImageCopy& copy,
		VkImageLayout dstImageLayout, VkPipelineStageFlags usageStageFlags, VkAccessFlags usageAccessFlags) {
		auto lock = std::lock_guard<std::shared_mutex>(m_accessMutex);
		AsyncImageTransfer transfer = {
			.stagingBufferAllocation = allocateStagingBufferArea(size),
			.dstImageHandle = dstImage,
			.copy = copy,
			.layoutTransitionBarrier = { .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
										 .srcAccessMask = 0,
										 .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
										 .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
										 .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
										 .srcQueueFamilyIndex = m_context->asyncTransferQueueFamilyIndex(),
										 .dstQueueFamilyIndex = m_context->graphicsQueueFamilyIndex(),
										 .image = m_resourceAllocator->nativeImageHandle(dstImage),
										 .subresourceRange = { .aspectMask = copy.imageSubresource.aspectMask,
															   .baseMipLevel = copy.imageSubresource.mipLevel,
															   .levelCount = 1,
															   .baseArrayLayer = copy.imageSubresource.baseArrayLayer,
															   .layerCount = copy.imageSubresource.layerCount,
															   } },
			.transferBarrier = { .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
								 .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
								 .dstAccessMask = 0,
								 .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
								 .newLayout = dstImageLayout,
								 .srcQueueFamilyIndex = m_context->asyncTransferQueueFamilyIndex(),
								 .dstQueueFamilyIndex = m_context->graphicsQueueFamilyIndex(),
								 .image = m_resourceAllocator->nativeImageHandle(dstImage),
								 .subresourceRange = { .aspectMask = copy.imageSubresource.aspectMask,
													   .baseMipLevel = copy.imageSubresource.mipLevel,
													   .levelCount = 1,
													   .baseArrayLayer = copy.imageSubresource.baseArrayLayer,
													   .layerCount = copy.imageSubresource.layerCount
													   } },
			.acquireBarrier = { .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
								.srcAccessMask = 0,
								.dstAccessMask = usageAccessFlags,
								.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
								.newLayout = dstImageLayout,
								.srcQueueFamilyIndex = m_context->asyncTransferQueueFamilyIndex(),
								.dstQueueFamilyIndex = m_context->graphicsQueueFamilyIndex(),
								.image = m_resourceAllocator->nativeImageHandle(dstImage),
								.subresourceRange = { .aspectMask = copy.imageSubresource.aspectMask,
													  .baseMipLevel = copy.imageSubresource.mipLevel,
													  .levelCount = 1,
													  .baseArrayLayer = copy.imageSubresource.baseArrayLayer,
													  .layerCount = copy.imageSubresource.layerCount,
													  } },
			.dstStageFlags = usageStageFlags
		};

		return m_asyncImageTransfers.addElement(transfer);
	}

	void GPUTransferManager::submitOneTimeTransfer(VkDeviceSize transferBufferSize, BufferResourceHandle handle,
												   const void* data, VkPipelineStageFlags usageStageFlags,
												   VkAccessFlags usageAccessFlags) {
		auto lock = std::lock_guard<std::shared_mutex>(m_accessMutex);
		GPUTransfer transfer = { .bufferSize = transferBufferSize,
								 .dstUsageStageFlags = usageStageFlags,
								 .dstUsageAccessFlags = usageAccessFlags };

		if (!m_resourceAllocator->bufferMemoryCapabilities(handle).hostVisible) {
			transfer.needsStagingBuffer = true;

			VkBufferCreateInfo transferBufferCreateInfo = { .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
															.size = transferBufferSize,
															.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
															.sharingMode = VK_SHARING_MODE_EXCLUSIVE };
			transfer.stagingBuffer = allocateStagingBufferArea(transferBufferSize);
			auto mappedDataStart =
				m_resourceAllocator->mappedBufferData(m_stagingBuffers[transfer.stagingBuffer.bufferHandle].buffer);
			std::memcpy(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(mappedDataStart) +
												transfer.stagingBuffer.allocationResult.usableRange.offset),
						data, transferBufferSize);
		} else {
			std::memcpy(m_resourceAllocator->mappedBufferData(handle), data, transferBufferSize);
		}

		transfer.dstBuffer = handle;
		m_oneTimeTransfers.push_back(transfer);
	}

	void GPUTransferManager::submitImageTransfer(ImageResourceHandle dstImage, const VkBufferImageCopy& copy,
												 const void* data, VkDeviceSize size,
												 VkPipelineStageFlags usageStageFlags, VkAccessFlags usageAccessFlags,
												 VkImageLayout dstUsageLayout) {
		auto lock = std::lock_guard<std::shared_mutex>(m_accessMutex);
		VkBufferCreateInfo transferBufferCreateInfo = { .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
														.size = size,
														.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
														.sharingMode = VK_SHARING_MODE_EXCLUSIVE };
		BufferResourceHandle buffer = m_resourceAllocator->createBuffer(
			transferBufferCreateInfo, { .hostVisible = true }, { .deviceLocal = true }, true);

		std::memcpy(m_resourceAllocator->mappedBufferData(buffer), data, size);

		m_imageTransfers.push_back({ .stagingBuffer = buffer,
									 .dstImage = dstImage,
									 .stagingBufferSize = size,
									 .copy = copy,
									 .dstUsageStageFlags = usageStageFlags,
									 .dstUsageAccessFlags = usageAccessFlags,
									 .dstUsageLayout = dstUsageLayout });
	}

	void GPUTransferManager::updateTransferData(GPUTransferHandle transferHandle, uint32_t frameIndex,
												const void* data) {
		auto lock = std::lock_guard<std::shared_mutex>(m_accessMutex);

		if (!m_bufferHandlesToInitiate.empty() || !m_imageHandlesToInitiate.empty()) {
			AsyncTransferCommandPoolHandle poolHandle;
			if (m_freeAsyncTransferCommandPools.empty()) {
				AsyncTransferCommandPool newPool = {};
				VkCommandPoolCreateInfo createInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
													   .queueFamilyIndex = m_context->asyncTransferQueueFamilyIndex() };
				verifyResult(vkCreateCommandPool(m_context->device(), &createInfo, nullptr, &newPool.pool));

				VkCommandBufferAllocateInfo allocateInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
															 .commandPool = newPool.pool,
															 .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
															 .commandBufferCount = 1 };
				verifyResult(vkAllocateCommandBuffers(m_context->device(), &allocateInfo, &newPool.buffer));

				VkFenceCreateInfo fenceCreateInfo = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
				verifyResult(vkCreateFence(m_context->device(), &fenceCreateInfo, nullptr, &newPool.fence));

				poolHandle = m_asyncTransferCommandPools.addElement(newPool);
			} else {
				vkResetFences(m_context->device(), 1, &m_freeAsyncTransferCommandPools.back().fence);
				poolHandle = m_asyncTransferCommandPools.addElement(m_freeAsyncTransferCommandPools.back());
				m_freeAsyncTransferCommandPools.pop_back();
			}
			m_asyncTransferCommandPools[poolHandle].refCount =
				m_bufferHandlesToInitiate.size() + m_imageHandlesToInitiate.size();
			VkCommandBufferBeginInfo beginInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
												   .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT };
			vkBeginCommandBuffer(m_asyncTransferCommandPools[poolHandle].buffer, &beginInfo);

			std::vector<VkImageMemoryBarrier> imageLayoutTransitionBarriers;
			imageLayoutTransitionBarriers.reserve(m_imageHandlesToInitiate.size());

			for (auto& handle : m_imageHandlesToInitiate) {
				auto& transfer = m_asyncImageTransfers[handle];
				imageLayoutTransitionBarriers.push_back(transfer.layoutTransitionBarrier);
			}
			vkCmdPipelineBarrier(m_asyncTransferCommandPools[poolHandle].buffer, 0, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
								 0, nullptr, 0, nullptr, static_cast<uint32_t>(imageLayoutTransitionBarriers.size()),
								 imageLayoutTransitionBarriers.data());

			std::vector<VkBufferMemoryBarrier> releaseBarriers;
			releaseBarriers.reserve(m_bufferHandlesToInitiate.size());
			for (auto& handle : m_bufferHandlesToInitiate) {
				auto& transfer = m_asyncBufferTransfers[handle];

				transfer.containingCommandPool = poolHandle;
				vkCmdCopyBuffer(m_asyncTransferCommandPools[poolHandle].buffer,
								m_resourceAllocator->nativeBufferHandle(
									m_stagingBuffers[transfer.stagingBufferAllocation.bufferHandle].buffer),
								m_resourceAllocator->nativeBufferHandle(transfer.dstBufferHandle), 1, &transfer.copy);
				releaseBarriers.push_back(transfer.transferBarrier);
			}

			std::vector<VkImageMemoryBarrier> imageReleaseBarriers;
			imageReleaseBarriers.reserve(m_imageHandlesToInitiate.size());
			for (auto& handle : m_imageHandlesToInitiate) {
				auto& transfer = m_asyncImageTransfers[handle];

				transfer.containingCommandPool = poolHandle;
				vkCmdCopyBufferToImage(m_asyncTransferCommandPools[poolHandle].buffer,
									   m_resourceAllocator->nativeBufferHandle(
										   m_stagingBuffers[transfer.stagingBufferAllocation.bufferHandle].buffer),
									   m_resourceAllocator->nativeImageHandle(transfer.dstImageHandle),
									   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &transfer.copy);
				imageReleaseBarriers.push_back(transfer.transferBarrier);
			}

			vkCmdPipelineBarrier(m_asyncTransferCommandPools[poolHandle].buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
								 0, nullptr, static_cast<uint32_t>(releaseBarriers.size()), releaseBarriers.data(),
								 static_cast<uint32_t>(imageReleaseBarriers.size()), imageReleaseBarriers.data());
			verifyResult(vkEndCommandBuffer(m_asyncTransferCommandPools[poolHandle].buffer));

			VkSubmitInfo transferSubmitInfo = { .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
												.commandBufferCount = 1,
												.pCommandBuffers = &m_asyncTransferCommandPools[poolHandle].buffer };
			verifyResult(vkQueueSubmit(m_context->asyncTransferQueue(), 1, &transferSubmitInfo,
									   m_asyncTransferCommandPools[poolHandle].fence));
		}

		auto& transfer = m_continuousTransfers[transferHandle][frameIndex];

		BufferResourceHandle dstWriteBuffer;
		void* dstData;
		if (transfer.needsStagingBuffer) {
			dstWriteBuffer = m_stagingBuffers[transfer.stagingBuffer.bufferHandle].buffer;
			dstData = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(m_resourceAllocator->mappedBufferData(
												  m_stagingBuffers[transfer.stagingBuffer.bufferHandle].buffer)) +
											  transfer.stagingBuffer.allocationResult.usableRange.offset);
		} else {
			dstWriteBuffer = transfer.dstBuffer;
			dstData = m_resourceAllocator->mappedBufferData(dstWriteBuffer);
		}

		std::memcpy(dstData, data, transfer.bufferSize);
		if (!m_resourceAllocator->bufferMemoryCapabilities(dstWriteBuffer).hostCoherent) {
			auto range = m_resourceAllocator->allocationRange(dstWriteBuffer);
			VkMappedMemoryRange flushRange = { .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
											   .memory = m_resourceAllocator->nativeMemoryHandle(dstWriteBuffer),
											   .offset = range.offset,
											   .size = range.size };
			vkFlushMappedMemoryRanges(m_context->device(), 1, &flushRange);
		}
	}

	BufferResourceHandle GPUTransferManager::dstBufferHandle(GPUTransferHandle handle, uint32_t frameIndex) {
		auto lock = SharedLockGuard(m_accessMutex);
		return m_continuousTransfers[handle][frameIndex].dstBuffer;
	}

	VkCommandBuffer GPUTransferManager::recordTransfers(uint32_t frameIndex) {
		auto lock = std::lock_guard<std::shared_mutex>(m_accessMutex);
		verifyResult(vkResetCommandPool(m_context->device(), m_transferCommandPools[frameIndex], 0));

		for (auto& bufferToFree : m_stagingBufferAllocationFreeList[frameIndex]) {
			freeToRanges(m_stagingBuffers[bufferToFree.bufferHandle].freeRangesOffsetSorted,
						 m_stagingBuffers[bufferToFree.bufferHandle].freeRangesSizeSorted,
						 bufferToFree.allocationResult.allocationRange.offset,
						 bufferToFree.allocationResult.allocationRange.size);
		}

		VkCommandBuffer commandBuffer = m_transferCommandBuffers[frameIndex];
		VkCommandBufferBeginInfo info = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
										  .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT };
		verifyResult(vkBeginCommandBuffer(commandBuffer, &info));

		std::vector<VkBufferMemoryBarrier> bufferBarriers;
		std::vector<VkImageMemoryBarrier> imageBarriers;
		bufferBarriers.reserve(m_continuousTransfers.size());
		imageBarriers.reserve(m_imageTransfers.size());

		VkPipelineStageFlags srcStageFlags = 0;

		for (auto& transfer : m_continuousTransfers) {
			if (transfer[frameIndex].needsStagingBuffer) {
				bufferBarriers.push_back(
					{ .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
					  .srcAccessMask = VK_ACCESS_HOST_WRITE_BIT,
					  .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
					  .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					  .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					  .buffer = m_resourceAllocator->nativeBufferHandle(
						  m_stagingBuffers[transfer[frameIndex].stagingBuffer.bufferHandle].buffer),
					  .offset = 0,
					  .size = VK_WHOLE_SIZE });
				srcStageFlags |= VK_PIPELINE_STAGE_HOST_BIT;
			}
		}
		for (auto& transfer : m_oneTimeTransfers) {
			if (transfer.needsStagingBuffer) {
				bufferBarriers.push_back({ .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
										   .srcAccessMask = VK_ACCESS_HOST_WRITE_BIT,
										   .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
										   .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
										   .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
										   .buffer = m_resourceAllocator->nativeBufferHandle(
											   m_stagingBuffers[transfer.stagingBuffer.bufferHandle].buffer),
										   .offset = 0,
										   .size = VK_WHOLE_SIZE });
				srcStageFlags |= VK_PIPELINE_STAGE_HOST_BIT;
			}
		}
		for (auto& transfer : m_imageTransfers) {
			imageBarriers.push_back(
				{ .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				  .srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
				  .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
				  .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				  .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				  .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				  .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				  .image = m_resourceAllocator->nativeImageHandle(transfer.dstImage),
				  .subresourceRange = { .aspectMask = transfer.copy.imageSubresource.aspectMask,
										.baseMipLevel = transfer.copy.imageSubresource.mipLevel,
										.levelCount = 1,
										.baseArrayLayer = transfer.copy.imageSubresource.baseArrayLayer,
										.layerCount = transfer.copy.imageSubresource.layerCount } });
			srcStageFlags |= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		}
		if (!bufferBarriers.empty() || !imageBarriers.empty()) {
			vkCmdPipelineBarrier(commandBuffer, srcStageFlags, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr,
								 static_cast<uint32_t>(bufferBarriers.size()), bufferBarriers.data(),
								 static_cast<uint32_t>(imageBarriers.size()), imageBarriers.data());
		}

		for (auto& transfer : m_continuousTransfers) {
			if (transfer[frameIndex].needsStagingBuffer) {
				VkBufferCopy copy = { .size = transfer[frameIndex].bufferSize };
				vkCmdCopyBuffer(commandBuffer,
								m_resourceAllocator->nativeBufferHandle(
									m_stagingBuffers[transfer[frameIndex].stagingBuffer.bufferHandle].buffer),
								m_resourceAllocator->nativeBufferHandle(transfer[frameIndex].dstBuffer), 1, &copy);
			}
		}
		for (auto& transfer : m_oneTimeTransfers) {
			if (transfer.needsStagingBuffer) {
				VkBufferCopy copy = { .size = transfer.bufferSize };
				vkCmdCopyBuffer(commandBuffer,
								m_resourceAllocator->nativeBufferHandle(
									m_stagingBuffers[transfer.stagingBuffer.bufferHandle].buffer),
								m_resourceAllocator->nativeBufferHandle(transfer.dstBuffer), 1, &copy);
			}
		}
		for (auto& transfer : m_imageTransfers) {
			vkCmdCopyBufferToImage(commandBuffer, m_resourceAllocator->nativeBufferHandle(transfer.stagingBuffer),
								   m_resourceAllocator->nativeImageHandle(transfer.dstImage),
								   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &transfer.copy);
		}

		bufferBarriers.clear();
		imageBarriers.clear();
		srcStageFlags = 0;
		VkPipelineStageFlags dstStageFlags = 0;

		for (auto& transfer : m_continuousTransfers) {
			VkBufferMemoryBarrier barrier = { .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
											  .srcAccessMask = VK_ACCESS_HOST_WRITE_BIT,
											  .dstAccessMask = transfer[frameIndex].dstUsageAccessFlags,
											  .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
											  .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
											  .buffer = m_resourceAllocator->nativeBufferHandle(
												  transfer[frameIndex].dstBuffer),
											  .offset = 0,
											  .size = VK_WHOLE_SIZE };
			if (transfer[frameIndex].needsStagingBuffer) {
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				srcStageFlags |= VK_PIPELINE_STAGE_TRANSFER_BIT;
			} else {
				srcStageFlags |= VK_PIPELINE_STAGE_HOST_BIT;
			}
			dstStageFlags |= transfer[frameIndex].dstUsageStageFlags;
			bufferBarriers.push_back(barrier);
		}
		for (auto& transfer : m_oneTimeTransfers) {
			VkBufferMemoryBarrier barrier = { .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
											  .srcAccessMask = VK_ACCESS_HOST_WRITE_BIT,
											  .dstAccessMask = transfer.dstUsageAccessFlags,
											  .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
											  .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
											  .buffer = m_resourceAllocator->nativeBufferHandle(transfer.dstBuffer),
											  .offset = 0,
											  .size = VK_WHOLE_SIZE };
			if (transfer.needsStagingBuffer) {
				m_stagingBufferAllocationFreeList[frameIndex].push_back(transfer.stagingBuffer);
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				srcStageFlags |= VK_PIPELINE_STAGE_TRANSFER_BIT;
			} else {
				srcStageFlags |= VK_PIPELINE_STAGE_HOST_BIT;
			}
			dstStageFlags |= transfer.dstUsageStageFlags;
			bufferBarriers.push_back(barrier);
		}
		for (auto& transfer : m_imageTransfers) {
			VkImageMemoryBarrier barrier = { .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
											 .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
											 .dstAccessMask = transfer.dstUsageAccessFlags,
											 .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
											 .newLayout = transfer.dstUsageLayout,
											 .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
											 .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
											 .image = m_resourceAllocator->nativeImageHandle(transfer.dstImage),
											 .subresourceRange = {
												 .aspectMask = transfer.copy.imageSubresource.aspectMask,
												 .baseMipLevel = transfer.copy.imageSubresource.mipLevel,
												 .levelCount = 1,
												 .baseArrayLayer = transfer.copy.imageSubresource.baseArrayLayer,
												 .layerCount = transfer.copy.imageSubresource.layerCount } };
			srcStageFlags |= VK_PIPELINE_STAGE_TRANSFER_BIT;
			dstStageFlags |= transfer.dstUsageStageFlags;
			m_resourceAllocator->destroyBuffer(transfer.stagingBuffer);
			imageBarriers.push_back(barrier);
		}
		vkCmdPipelineBarrier(commandBuffer, srcStageFlags, dstStageFlags, 0, 0, nullptr,
							 static_cast<uint32_t>(bufferBarriers.size()), bufferBarriers.data(),
							 static_cast<uint32_t>(imageBarriers.size()), imageBarriers.data());

		verifyResult(vkEndCommandBuffer(commandBuffer));

		m_imageTransfers.clear();
		m_oneTimeTransfers.clear();
		return commandBuffer;
	}

	void GPUTransferManager::destroy() {
		for (auto& pool : m_transferCommandPools) {
			vkDestroyCommandPool(m_context->device(), pool, nullptr);
		}
	}

	StagingBufferAllocation GPUTransferManager::allocateStagingBufferArea(VkDeviceSize size) {
		for (auto iterator = m_stagingBuffers.begin(); iterator != m_stagingBuffers.end(); ++iterator) {
			auto& block = *iterator;
			bool isCoherent = m_resourceAllocator->bufferMemoryCapabilities(block.buffer).hostCoherent;
			auto allocResult =
				allocateFromRanges(block.freeRangesOffsetSorted, block.freeRangesSizeSorted,
								   isCoherent ? 0 : m_context->properties().limits.nonCoherentAtomSize, size);
			if (allocResult.has_value()) {
				return { .bufferHandle = m_stagingBuffers.handle(iterator), .allocationResult = allocResult.value() };
			}
		}

		VkBufferCreateInfo newBufferCreateInfo = { .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
												   .size = std::max(m_minStagingBlockSize, size),
												   .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
												   .sharingMode = VK_SHARING_MODE_EXCLUSIVE };
		StagingBuffer newBuffer = { .buffer = m_resourceAllocator->createBuffer(
										newBufferCreateInfo, { .hostVisible = true }, { .hostCoherent = true }, true),
									.maxAllocatableSize = newBufferCreateInfo.size,
									.originalSize = newBufferCreateInfo.size,
									.freeRangesOffsetSorted = { { .offset = 0, .size = newBufferCreateInfo.size } },
									.freeRangesSizeSorted = { { .offset = 0, .size = newBufferCreateInfo.size } } };

		bool isCoherent = m_resourceAllocator->bufferMemoryCapabilities(newBuffer.buffer).hostCoherent;
		return { .bufferHandle = m_stagingBuffers.addElement(newBuffer),
				 .allocationResult =
					 allocateFromRanges((--m_stagingBuffers.end())->freeRangesOffsetSorted,
										(--m_stagingBuffers.end())->freeRangesSizeSorted,
										isCoherent ? 0 : m_context->properties().limits.nonCoherentAtomSize, size)
						 .value() };
	}

	void GPUTransferManager::tryCleanupStagingBuffers() {
		auto lock = std::lock_guard<std::shared_mutex>(m_accessMutex);
	blockFreeStart:
		auto blockIterator = m_stagingBuffers.begin();
		for (auto& block : m_stagingBuffers) {
			if (block.maxAllocatableSize == block.originalSize) {
				m_resourceAllocator->destroyBufferImmediately(block.buffer);
				m_stagingBuffers.removeElement(m_stagingBuffers.handle(blockIterator));
				goto blockFreeStart;
			}
			++blockIterator;
		}
	}

	bool GPUTransferManager::isBufferTransferFinished(AsyncBufferTransferHandle handle) {
		auto lock = SharedLockGuard(m_accessMutex);
		auto& transfer = m_asyncBufferTransfers[handle];

		if (transfer.containingCommandPool == ~0U) {
			return false;
		}
		VkResult status =
			vkGetFenceStatus(m_context->device(), m_asyncTransferCommandPools[transfer.containingCommandPool].fence);

		switch (status) {
			case VK_SUCCESS:
				return true;
				break;
			case VK_NOT_READY:
				return false;
			default:
				verifyResult(status);
				return false;
				break;
		}
	}

	bool GPUTransferManager::isImageTransferFinished(AsyncImageTransferHandle handle) {
		auto lock = SharedLockGuard(m_accessMutex);
		auto& transfer = m_asyncImageTransfers[handle];

		if (transfer.containingCommandPool == ~0U) {
			return false;
		}
		VkResult status =
			vkGetFenceStatus(m_context->device(), m_asyncTransferCommandPools[transfer.containingCommandPool].fence);

		switch (status) {
			case VK_SUCCESS:
				return true;
				break;
			case VK_NOT_READY:
				return false;
			default:
				verifyResult(status);
				return false;
				break;
		}
	}

	void GPUTransferManager::finalizeAsyncBufferTransfer(AsyncBufferTransferHandle handle) {
		auto lock = std::lock_guard<std::shared_mutex>(m_accessMutex);
		m_bufferFinalizationBarriers.push_back(m_asyncBufferTransfers[handle].acquireBarrier);

		auto bufferHandle = m_asyncBufferTransfers[handle].stagingBufferAllocation.bufferHandle;
		auto& allocationRange = m_asyncBufferTransfers[handle].stagingBufferAllocation.allocationResult.allocationRange;
		freeToRanges(m_stagingBuffers[bufferHandle].freeRangesOffsetSorted,
					 m_stagingBuffers[bufferHandle].freeRangesSizeSorted, allocationRange.offset, allocationRange.size);
		m_asyncBufferTransfers.removeElement(handle);
	}

	void GPUTransferManager::finalizeAsyncImageTransfer(AsyncImageTransferHandle handle) {
		auto lock = std::lock_guard<std::shared_mutex>(m_accessMutex);
		m_imageFinalizationBarriers.push_back(m_asyncImageTransfers[handle].acquireBarrier);

		auto bufferHandle = m_asyncImageTransfers[handle].stagingBufferAllocation.bufferHandle;
		auto& allocationRange = m_asyncImageTransfers[handle].stagingBufferAllocation.allocationResult.allocationRange;
		freeToRanges(m_stagingBuffers[bufferHandle].freeRangesOffsetSorted,
					 m_stagingBuffers[bufferHandle].freeRangesSizeSorted, allocationRange.offset, allocationRange.size);
		m_asyncImageTransfers.removeElement(handle);
	}
} // namespace vanadium::graphics