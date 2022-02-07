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

struct VGPUImageTransfer {
	VBufferResourceHandle stagingBuffer;
	VImageResourceHandle dstImage;
	VkDeviceSize stagingBufferSize;

	VkBufferImageCopy copy;
	VkPipelineStageFlags dstUsageStageFlags;
	VkAccessFlags dstUsageAccessFlags;
	VkImageLayout dstUsageLayout;
};

using VGPUTransferHandle = VSlotmapHandle;

class VGPUTransferManager {
  public:
	VGPUTransferManager() {}

	void create(VGPUContext* context, VGPUResourceAllocator* allocator);

	VGPUTransferHandle createTransfer(VkDeviceSize transferBufferSize, VkBufferUsageFlags usageFlags,
									  VkPipelineStageFlags usageStageFlags, VkAccessFlags usageAccessFlags);

	void submitOneTimeTransfer(VkDeviceSize transferBufferSize, VBufferResourceHandle handle, const void* data,
											 VkPipelineStageFlags usageStageFlags, VkAccessFlags usageAccessFlags);

	void submitImageTransfer(VImageResourceHandle dstImage, const VkBufferImageCopy& copy, const void* data,
							 VkDeviceSize size, VkPipelineStageFlags usageStageFlags, VkAccessFlags usageAccessFlags,
							 VkImageLayout dstUsageLayout);

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
	std::vector<VGPUTransfer> m_oneTimeTransfers;
	std::vector<VGPUImageTransfer> m_imageTransfers;
};