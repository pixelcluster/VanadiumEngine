#include <modules/gpu/helper/ErrorHelper.hpp>
#include <modules/gpu/transfer/VGPUTransferManager.hpp>
#include <volk.h>

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
		buffer = m_resourceAllocator->createBuffer(transferBufferCreateInfo, {}, { .deviceLocal = true }, false);
	}
	transfer.dstBuffer = buffer;

	return m_continuousTransfers.addElement(transfer);
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

	std::vector<VkBufferMemoryBarrier> copyBarriers;
	copyBarriers.reserve(m_continuousTransfers.size());

	for (auto& transfer : m_continuousTransfers) {
		if (transfer.needsStagingBuffer) {
			copyBarriers.push_back({ .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
										.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT,
										.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
										.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
										.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
										.buffer = m_resourceAllocator->nativeBufferHandle(transfer.stagingBuffer),
										.offset = 0,
										.size = VK_WHOLE_SIZE });
		}
	}
	if (!copyBarriers.empty()) {
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr,
							 copyBarriers.size(), copyBarriers.data(), 0, nullptr);
	}
	VkBufferCopy copy = { .size = VK_WHOLE_SIZE };
	for (auto& transfer : m_continuousTransfers) {
		if (transfer.needsStagingBuffer) {
			vkCmdCopyBuffer(commandBuffer, m_resourceAllocator->nativeBufferHandle(transfer.stagingBuffer),
							m_resourceAllocator->nativeBufferHandle(transfer.dstBuffer), 1, &copy);
		}
	}

	copyBarriers.clear();
	VkPipelineStageFlags srcStageFlags = 0;
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
		copyBarriers.push_back(barrier);
	}
	vkCmdPipelineBarrier(commandBuffer, srcStageFlags, dstStageFlags, 0, 0, nullptr,
						 copyBarriers.size(), copyBarriers.data(), 0, nullptr);

	verifyResult(vkEndCommandBuffer(commandBuffer));
	return commandBuffer;
}

void VGPUTransferManager::destroy() {
	for (auto& pool : m_transferCommandPools) {
		vkDestroyCommandPool(m_context->device(), pool, nullptr);
	}
}
