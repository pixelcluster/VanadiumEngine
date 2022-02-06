#pragma once

#include <helper/VSlotmap.hpp>
#include <modules/gpu/VGPUContext.hpp>
#include <modules/gpu/VGPUResourceAllocator.hpp>

struct VGPUTransfer {
	VBufferResourceHandle dstBuffer;
	VBufferResourceHandle stagingBuffer;
	bool needsStagingBuffer;
	VkDeviceSize bufferSize;

	VkPipelineStageFlags dstUsageStageFlags;
	VkAccessFlags dstUsageAccessFlags;
};

using VGPUTransferHandle = VSlotmapHandle;

class VGPUTransferManager {
  public:
	VGPUTransferManager() {}

	void create(VGPUContext* context, VGPUResourceAllocator* allocator);

	VGPUTransferHandle createTransfer(VkDeviceSize transferBufferSize, VkBufferUsageFlags usageFlags,
									  VkPipelineStageFlags usageStageFlags, VkAccessFlags usageAccessFlags);

	VBufferResourceHandle dstBufferHandle(VGPUTransferHandle handle) { return m_continuousTransfers[handle].dstBuffer; }

	void updateTransferData(VGPUTransferHandle transfer, const void* data);

	VkCommandBuffer recordTransfers(uint32_t frameIndex);

	void destroy();

  private:
	VGPUContext* m_context;
	VGPUResourceAllocator* m_resourceAllocator;

	VkCommandBuffer m_transferCommandBuffers[frameInFlightCount];
	VkCommandPool m_transferCommandPools[frameInFlightCount];

	VSlotmap<VGPUTransfer> m_continuousTransfers;
};