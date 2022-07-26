/* VanadiumEngine, a Vulkan rendering toolkit
 * Copyright (C) 2022 Friedrich Vock
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#pragma once

#include <Slotmap.hpp>
#include <graphics/DeviceContext.hpp>
#include <graphics/util/GPUResourceAllocator.hpp>
#include <graphics/util/RangeAllocator.hpp>
#include <shared_mutex>
#include <util/MemoryLiterals.hpp>

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
		std::vector<StagingBufferAllocation> stagingBuffers;
		std::vector<bool> hasNewData;
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
		VkImageLayout srcLayout;
	};

	using GPUTransferHandle = SlotmapHandle;

	struct AsyncTransferCommandPool {
		uint32_t refCount = 0;
		VkCommandPool pool;
		VkCommandBuffer buffer;
		VkFence fence;
	};

	using AsyncTransferCommandPoolHandle = SlotmapHandle;

	struct AsyncBufferTransfer {
		StagingBufferAllocation stagingBufferAllocation;
		BufferResourceHandle dstBufferHandle;

		VkBufferCopy copy;
		VkBufferMemoryBarrier transferBarrier;
		VkBufferMemoryBarrier acquireBarrier;
		VkPipelineStageFlags dstStageFlags;

		AsyncTransferCommandPoolHandle containingCommandPool = ~0U;
	};

	using AsyncBufferTransferHandle = SlotmapHandle;

	struct AsyncImageTransfer {
		StagingBufferAllocation stagingBufferAllocation;
		ImageResourceHandle dstImageHandle;

		VkBufferImageCopy copy;
		VkImageMemoryBarrier layoutTransitionBarrier;
		VkImageMemoryBarrier transferBarrier;
		VkImageMemoryBarrier acquireBarrier;
		VkPipelineStageFlags dstStageFlags;

		AsyncTransferCommandPoolHandle containingCommandPool = ~0U;
	};

	using AsyncImageTransferHandle = SlotmapHandle;

	class GPUTransferManager {
	  public:
		GPUTransferManager() {}

		void create(DeviceContext* context, GPUResourceAllocator* allocator);

		// Creates a GPU transfer. This automatically allocates one destination buffer per frame in flight and
		// corresponding staging buffers, if the destination buffer cannot be allocated in host-visible VRAM.
		GPUTransferHandle createTransfer(VkDeviceSize transferBufferSize, VkBufferUsageFlags usageFlags,
										 VkPipelineStageFlags usageStageFlags, VkAccessFlags usageAccessFlags);

		void destroyTransfer(GPUTransferHandle handle);

		// Transmits data to a buffer using the asynchronous transfer queue of the device, if any exists.
		AsyncBufferTransferHandle createAsyncBufferTransfer(void* data, size_t size, BufferResourceHandle dstBuffer,
															size_t offset, VkPipelineStageFlags usageStageFlags,
															VkAccessFlags usageAccessFlags);

		// Transmits data to an image using the asynchronous transfer queue of the device, if any exists.
		AsyncImageTransferHandle createAsyncImageTransfer(void* data, size_t size, ImageResourceHandle dstImage,
														  const VkBufferImageCopy& copy, VkImageLayout dstImageLayout,
														  VkPipelineStageFlags usageStageFlags,
														  VkAccessFlags usageAccessFlags);

		void submitOneTimeTransfer(VkDeviceSize transferBufferSize, BufferResourceHandle handle, const void* data,
								   VkPipelineStageFlags usageStageFlags, VkAccessFlags usageAccessFlags);

		void submitImageTransfer(ImageResourceHandle dstImage, const VkBufferImageCopy& copy, const void* data,
								 VkDeviceSize size, VkPipelineStageFlags usageStageFlags,
								 VkAccessFlags usageAccessFlags, VkImageLayout dstUsageLayout,
								 VkImageLayout srcLayout = VK_IMAGE_LAYOUT_UNDEFINED);

		BufferResourceHandle dstBufferHandle(GPUTransferHandle handle);

		void updateTransferData(GPUTransferHandle transfer, uint32_t frameIndex, const void* data);

		VkCommandBuffer recordTransfers(uint32_t frameIndex);

		void destroy();
		void tryCleanupStagingBuffers();

		bool isBufferTransferFinished(AsyncBufferTransferHandle transferHandle);
		bool isImageTransferFinished(AsyncImageTransferHandle transferHandle);

		void finalizeAsyncBufferTransfer(AsyncBufferTransferHandle transferHandle);
		void finalizeAsyncImageTransfer(AsyncImageTransferHandle transferHandle);

		StagingBufferAllocation allocateStagingBufferArea(VkDeviceSize size);

	  private:
		constexpr static size_t m_minStagingBlockSize = 32_MiB;

		DeviceContext* m_context;
		GPUResourceAllocator* m_resourceAllocator;

		Slotmap<AsyncTransferCommandPool> m_asyncTransferCommandPools;
		std::vector<AsyncTransferCommandPool> m_freeAsyncTransferCommandPools;

		VkDeviceSize m_nonCoherentAtomSize;

		VkCommandBuffer m_transferCommandBuffers[frameInFlightCount];
		VkCommandPool m_transferCommandPools[frameInFlightCount];

		Slotmap<GPUTransfer> m_continuousTransfers;
		std::vector<GPUTransfer> m_oneTimeTransfers;
		std::vector<GPUImageTransfer> m_imageTransfers;

		Slotmap<AsyncBufferTransfer> m_asyncBufferTransfers;
		Slotmap<AsyncImageTransfer> m_asyncImageTransfers;

		// Handles of async buffer transfers whose commands need to be submitted to the async transfer queue.
		std::vector<AsyncBufferTransferHandle> m_bufferHandlesToBegin;
		// Handles of async image transfers whose commands need to be submitted to the async transfer queue.
		std::vector<AsyncImageTransferHandle> m_imageHandlesToBegin;

		// Barriers performing Queue Family Ownership Transfers from the asynchronous copy queue to the destination
		// queue.
		std::vector<VkBufferMemoryBarrier> m_bufferFinalizationBarriers;
		// Barriers performing Queue Family Ownership Transfers from the asynchronous copy queue to the destination
		// queue.
		std::vector<VkImageMemoryBarrier> m_imageFinalizationBarriers;

		Slotmap<StagingBuffer> m_stagingBuffers;

		// An array of free staging buffers for each frame in flight.
		std::vector<std::vector<StagingBufferAllocation>> m_stagingBufferAllocationFreeList;

		std::shared_mutex m_accessMutex;
	};
} // namespace vanadium::graphics
