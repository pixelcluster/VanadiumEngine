#pragma once

#include <helper/Slotmap.hpp>
#include <graphics/DeviceContext.hpp>
#include <graphics/util/GPUResourceAllocator.hpp>

namespace vanadium::graphics {

	struct GPUTransfer {
		BufferResourceHandle dstBuffer;
		BufferResourceHandle stagingBuffer;
		bool needsStagingBuffer;
		VkDeviceSize bufferSize;

		VkPipelineStageFlags dstUsageStageFlags;
		VkAccessFlags dstUsageAccessFlags;
	};

	struct VGPUImageTransfer {
		BufferResourceHandle stagingBuffer;
		ImageResourceHandle dstImage;
		VkDeviceSize stagingBufferSize;

		VkBufferImageCopy copy;
		VkPipelineStageFlags dstUsageStageFlags;
		VkAccessFlags dstUsageAccessFlags;
		VkImageLayout dstUsageLayout;
	};

	using GPUTransferHandle = SlotmapHandle<GPUTransfer>;

	class GPUTransferManager {
	  public:
		GPUTransferManager() {}

		void create(DeviceContext* context, GPUResourceAllocator* allocator);

		GPUTransferHandle createTransfer(VkDeviceSize transferBufferSize, VkBufferUsageFlags usageFlags,
										  VkPipelineStageFlags usageStageFlags, VkAccessFlags usageAccessFlags);

		void submitOneTimeTransfer(VkDeviceSize transferBufferSize, BufferResourceHandle handle, const void* data,
								   VkPipelineStageFlags usageStageFlags, VkAccessFlags usageAccessFlags);

		void submitImageTransfer(ImageResourceHandle dstImage, const VkBufferImageCopy& copy, const void* data,
								 VkDeviceSize size, VkPipelineStageFlags usageStageFlags,
								 VkAccessFlags usageAccessFlags, VkImageLayout dstUsageLayout);

		BufferResourceHandle dstBufferHandle(GPUTransferHandle handle) {
			return m_continuousTransfers[handle].dstBuffer;
		}

		void updateTransferData(GPUTransferHandle transfer, const void* data);

		VkCommandBuffer recordTransfers(uint32_t frameIndex);

		void destroy();

	  private:
		DeviceContext* m_context;
		GPUResourceAllocator* m_resourceAllocator;

		VkCommandBuffer m_transferCommandBuffers[frameInFlightCount];
		VkCommandPool m_transferCommandPools[frameInFlightCount];

		Slotmap<GPUTransfer> m_continuousTransfers;
		std::vector<GPUTransfer> m_oneTimeTransfers;
		std::vector<VGPUImageTransfer> m_imageTransfers;
	};
} // namespace vanadium::graphics