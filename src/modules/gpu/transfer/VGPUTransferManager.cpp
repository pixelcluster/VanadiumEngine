#include <modules/gpu/helper/ErrorHelper.hpp>
#include <modules/gpu/transfer/VGPUTransferManager.hpp>
#include <volk.h>
#include <cstring>

void VGPUTransferManager::create(VGPUContext* context, VGPUResourceAllocator* allocator) {
	m_context = context;
	m_resourceAllocator = allocator;

	VkCommandPoolCreateInfo poolCreateInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
											   .queueFamilyIndex = m_context->graphicsQueueFamilyIndex() };

	for (size_t i = 0; i < frameInFlightCount; ++i) {
		verifyResult(vkCreateCommandPool(m_context->device(), &poolCreateInfo, nullptr, &m_transferCommandPools[i]));

		VkCommandBufferAllocateInfo allocateInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
													 .commandPool = m_transferCommandPools[i],
													 .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
													 .commandBufferCount = 1 };
		verifyResult(vkAllocateCommandBuffers(m_context->device(), &allocateInfo, &m_transferCommandBuffers[i]));
	}
}

VGPUTransferHandle VGPUTransferManager::createTransfer(VkDeviceSize transferBufferSize, VkBufferUsageFlags usageFlags,
													   VkPipelineStageFlags usageStageFlags,
													   VkAccessFlags usageAccessFlags) {
	VkBufferCreateInfo transferBufferCreateInfo = { .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
													.size = transferBufferSize,
													.usage = usageFlags | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
													.sharingMode = VK_SHARING_MODE_EXCLUSIVE };
	VBufferResourceHandle buffer =
		m_resourceAllocator->createBuffer(transferBufferCreateInfo, {}, { .deviceLocal = true }, true);

	VGPUTransfer transfer = { .bufferSize = transferBufferSize,
							  .dstUsageStageFlags = usageStageFlags,
							  .dstUsageAccessFlags = usageAccessFlags };

	if (!m_resourceAllocator->bufferMemoryCapabilities(buffer).deviceLocal) {
		transfer.needsStagingBuffer = true;
		transfer.stagingBuffer = buffer;
		transferBufferCreateInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		buffer = m_resourceAllocator->createBuffer(transferBufferCreateInfo, {}, { .deviceLocal = true }, false);
	}
	transfer.dstBuffer = buffer;

	return m_continuousTransfers.addElement(transfer);
}

void VGPUTransferManager::submitOneTimeTransfer(VkDeviceSize transferBufferSize, VBufferResourceHandle handle,
												const void* data, VkPipelineStageFlags usageStageFlags,
												VkAccessFlags usageAccessFlags) {
	VGPUTransfer transfer = { .bufferSize = transferBufferSize,
							  .dstUsageStageFlags = usageStageFlags,
							  .dstUsageAccessFlags = usageAccessFlags };

	if (!m_resourceAllocator->bufferMemoryCapabilities(handle).hostVisible) {
		transfer.needsStagingBuffer = true;

		VkBufferCreateInfo transferBufferCreateInfo = { .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
														.size = transferBufferSize,
														.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
														.sharingMode = VK_SHARING_MODE_EXCLUSIVE };
		transfer.stagingBuffer =
			m_resourceAllocator->createBuffer(transferBufferCreateInfo, {}, { .deviceLocal = true }, true);
		std::memcpy(m_resourceAllocator->mappedBufferData(transfer.stagingBuffer), data, transferBufferSize);
	} else {
		std::memcpy(m_resourceAllocator->mappedBufferData(handle), data, transferBufferSize);
	}

	transfer.dstBuffer = handle;
	m_oneTimeTransfers.push_back(transfer);
}

void VGPUTransferManager::submitImageTransfer(VImageResourceHandle dstImage, const VkBufferImageCopy& copy,
											  const void* data, VkDeviceSize size, VkPipelineStageFlags usageStageFlags,
											  VkAccessFlags usageAccessFlags, VkImageLayout dstUsageLayout) {
	VkBufferCreateInfo transferBufferCreateInfo = { .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
													.size = size,
													.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
													.sharingMode = VK_SHARING_MODE_EXCLUSIVE };
	VBufferResourceHandle buffer =
		m_resourceAllocator->createBuffer(transferBufferCreateInfo, {}, { .deviceLocal = true }, true);

	std::memcpy(m_resourceAllocator->mappedBufferData(buffer), data, size);

	m_imageTransfers.push_back({ .stagingBuffer = buffer,
								 .dstImage = dstImage,
								 .stagingBufferSize = size,
								 .copy = copy,
								 .dstUsageStageFlags = usageStageFlags,
								 .dstUsageAccessFlags = usageAccessFlags,
								 .dstUsageLayout = dstUsageLayout });
}

void VGPUTransferManager::updateTransferData(VGPUTransferHandle transferHandle, const void* data) {
	auto& transfer = m_continuousTransfers[transferHandle];

	VBufferResourceHandle dstWriteBuffer;
	if (transfer.needsStagingBuffer) {
		dstWriteBuffer = transfer.stagingBuffer;
	} else {
		dstWriteBuffer = transfer.dstBuffer;
	}

	std::memcpy(m_resourceAllocator->mappedBufferData(dstWriteBuffer), data, transfer.bufferSize);
	if (!m_resourceAllocator->bufferMemoryCapabilities(dstWriteBuffer).hostCoherent) {
		auto range = m_resourceAllocator->allocationRange(dstWriteBuffer);
		VkMappedMemoryRange flushRange = { .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
										   .memory = m_resourceAllocator->nativeMemoryHandle(dstWriteBuffer),
										   .offset = range.offset,
										   .size = range.size };
		vkFlushMappedMemoryRanges(m_context->device(), 1, &flushRange);
	}
}

VkCommandBuffer VGPUTransferManager::recordTransfers(uint32_t frameIndex) {
	verifyResult(vkResetCommandPool(m_context->device(), m_transferCommandPools[frameIndex], 0));

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
		if (transfer.needsStagingBuffer) {
			bufferBarriers.push_back({ .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
									   .srcAccessMask = VK_ACCESS_HOST_WRITE_BIT,
									   .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
									   .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
									   .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
									   .buffer = m_resourceAllocator->nativeBufferHandle(transfer.stagingBuffer),
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
									   .buffer = m_resourceAllocator->nativeBufferHandle(transfer.stagingBuffer),
									   .offset = 0,
									   .size = VK_WHOLE_SIZE });
			srcStageFlags |= VK_PIPELINE_STAGE_HOST_BIT;
		}
	}
	for (auto& transfer : m_imageTransfers) {
		imageBarriers.push_back({ .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
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
		if (transfer.needsStagingBuffer) {
			VkBufferCopy copy = { .size = transfer.bufferSize };
			vkCmdCopyBuffer(commandBuffer, m_resourceAllocator->nativeBufferHandle(transfer.stagingBuffer),
							m_resourceAllocator->nativeBufferHandle(transfer.dstBuffer), 1, &copy);
		}
	}
	for (auto& transfer : m_oneTimeTransfers) {
		if (transfer.needsStagingBuffer) {
			VkBufferCopy copy = { .size = transfer.bufferSize };
			vkCmdCopyBuffer(commandBuffer, m_resourceAllocator->nativeBufferHandle(transfer.stagingBuffer),
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
										  .dstAccessMask = transfer.dstUsageAccessFlags,
										  .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
										  .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
										  .buffer = m_resourceAllocator->nativeBufferHandle(transfer.stagingBuffer),
										  .offset = 0,
										  .size = VK_WHOLE_SIZE };
		if (transfer.needsStagingBuffer) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			srcStageFlags |= VK_PIPELINE_STAGE_TRANSFER_BIT;
		} else {
			srcStageFlags |= VK_PIPELINE_STAGE_HOST_BIT;
		}
		dstStageFlags |= transfer.dstUsageStageFlags;
		bufferBarriers.push_back(barrier);
	}
	for (auto& transfer : m_oneTimeTransfers) {
		VkBufferMemoryBarrier barrier = { .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
										  .srcAccessMask = VK_ACCESS_HOST_WRITE_BIT,
										  .dstAccessMask = transfer.dstUsageAccessFlags,
										  .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
										  .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
										  .buffer = m_resourceAllocator->nativeBufferHandle(transfer.stagingBuffer),
										  .offset = 0,
										  .size = VK_WHOLE_SIZE };
		if (transfer.needsStagingBuffer) {
			m_resourceAllocator->destroyBuffer(transfer.stagingBuffer);
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

void VGPUTransferManager::destroy() {
	for (auto& pool : m_transferCommandPools) {
		vkDestroyCommandPool(m_context->device(), pool, nullptr);
	}
}
