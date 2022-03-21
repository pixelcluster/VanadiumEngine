#pragma once

#include <graphics/DeviceContext.hpp>
#include <graphics/util/GPUResourceAllocator.hpp>
#include <graphics/util/RangeAllocator.hpp>
#include <helper/Slotmap.hpp>
#include <helper/MemoryLiterals.hpp>
#include <shared_mutex>

namespace vanadium::graphics {

	struct StagingBuffer {
		BufferResourceHandle buffer;
		VkDeviceSize maxAllocatableSize;
		VkDeviceSize originalSize;
		
		std::vector<MemoryRange> freeRangesOffsetSorted;
		std::vector<MemoryRange> freeRangesSizeSorted;
	};

	using StagingBufferHandle = SlotmapHandle;

	struct StagingBufferAllocation {
		StagingBufferHandle bufferHandle;
		RangeAllocationResult allocationResult;
	};

	struct GPUTransfer {
		BufferResourceHandle dstBuffer;
		StagingBufferAllocation stagingBuffer;
		bool needsStagingBuffer;
		VkDeviceSize bufferSize;

		VkPipelineStageFlags dstUsageStageFlags;
		VkAccessFlags dstUsageAccessFlags;
	};

	struct GPUImageTransfer {
		BufferResourceHandle stagingBuffer;
		ImageResourceHandle dstImage;
		VkDeviceSize stagingBufferSize;

		VkBufferImageCopy copy;
		VkPipelineStageFlags dstUsageStageFlags;
		VkAccessFlags dstUsageAccessFlags;
		VkImageLayout dstUsageLayout;
	};

	using GPUTransferHandle = SlotmapHandle;

	struct AsyncBufferTransfer
	{
		StagingBufferAllocation stagingBufferAllocation;
		BufferResourceHandle dstBufferHandle;

		VkBufferCopy copy;
		VkBufferMemoryBarrier transferBarrier;
		VkPipelineStageFlags dstStageFlags;
		VkEvent finishedEvent;
	};

	using AsyncBufferTransferHandle = SlotmapHandle;
	using AsyncImageTransferHandle = SlotmapHandle;

	class GPUTransferManager {
	  public:
		GPUTransferManager() {}

		void create(DeviceContext* context, GPUResourceAllocator* allocator);

		GPUTransferHandle createTransfer(VkDeviceSize transferBufferSize, VkBufferUsageFlags usageFlags,
										 VkPipelineStageFlags usageStageFlags, VkAccessFlags usageAccessFlags);

		AsyncBufferTransferHandle createAsyncBufferTransfer(void* data, size_t size, BufferResourceHandle dstBuffer,
															size_t offset, VkPipelineStageFlags usageStageFlags,
															VkAccessFlags usageAccessFlags);

		AsyncImageTransferHandle createAsyncImageTransfer(void* data, size_t size, ImageResourceHandle dstImage,
														  uint32_t mipLevel, uint32_t arrayLayer, VkOffset3D offset, VkExtent3D extent,
														  VkPipelineStageFlags usageStageFlags,
														  VkAccessFlags usageAccessFlags);

		void submitOneTimeTransfer(VkDeviceSize transferBufferSize, BufferResourceHandle handle, const void* data,
								   VkPipelineStageFlags usageStageFlags, VkAccessFlags usageAccessFlags);

		void submitImageTransfer(ImageResourceHandle dstImage, const VkBufferImageCopy& copy, const void* data,
								 VkDeviceSize size, VkPipelineStageFlags usageStageFlags,
								 VkAccessFlags usageAccessFlags, VkImageLayout dstUsageLayout);

		BufferResourceHandle dstBufferHandle(GPUTransferHandle handle, uint32_t frameIndex);

		void updateTransferData(GPUTransferHandle transfer, uint32_t frameIndex, const void* data);

		VkCommandBuffer recordTransfers(uint32_t frameIndex);

		void destroy();
		void tryCleanupStagingBuffers();

		bool isBufferTransferFinished(AsyncBufferTransferHandle transferHandle);
		bool isImageTransferFinished(AsyncImageTransferHandle transferHandle);

		void destroyAsyncBufferTransfer(AsyncBufferTransferHandle transferHandle);
		void destroyAsyncImageTransfer(AsyncImageTransferHandle transferHandle);

		StagingBufferAllocation allocateStagingBufferArea(VkDeviceSize size);

	  private:

		constexpr static size_t m_minStagingBlockSize = 32_MiB;

		DeviceContext* m_context;
		GPUResourceAllocator* m_resourceAllocator;

		VkDeviceSize m_nonCoherentAtomSize;

		VkCommandBuffer m_transferCommandBuffers[frameInFlightCount];
		VkCommandPool m_transferCommandPools[frameInFlightCount];

		Slotmap<std::vector<GPUTransfer>> m_continuousTransfers;
		std::vector<GPUTransfer> m_oneTimeTransfers;
		std::vector<GPUImageTransfer> m_imageTransfers;

		Slotmap<AsyncBufferTransfer> m_asyncBufferTransfers;

		Slotmap<StagingBuffer> m_stagingBuffers;

		std::vector<std::vector<StagingBufferAllocation>> m_stagingBufferAllocationFreeList;
		
		std::shared_mutex m_accessMutex;
	};
} // namespace vanadium::graphics