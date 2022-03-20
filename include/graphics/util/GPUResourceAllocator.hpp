#pragma once

#include <array>
#include <graphics/DeviceContext.hpp>
#include <helper/MemoryLiterals.hpp>
#include <helper/Slotmap.hpp>

namespace vanadium::graphics {

	struct MemoryCapabilities {
		bool deviceLocal;
		bool hostVisible;
		bool hostCoherent;
	};

	struct MemoryRange {
		VkDeviceSize offset;
		VkDeviceSize size;
	};

	struct MemoryBlock {
		std::vector<MemoryRange> freeBlocksSizeSorted;
		std::vector<MemoryRange> freeBlocksOffsetSorted;

		MemoryCapabilities capabilities;

		VkDeviceSize maxAllocatableSize;
		VkDeviceSize originalSize;

		VkDeviceMemory memoryHandle;
		void* mappedPointer;
	};

	using BlockHandle = SlotmapHandle;

	struct MemoryType {
		VkMemoryPropertyFlags properties;
		uint32_t heapIndex;

		Slotmap<MemoryBlock> blocks;
		Slotmap<MemoryBlock> imageBlocks;
	};

	struct BufferAllocation {
		bool isMultipleBuffered = false;

		uint32_t typeIndex;
		BlockHandle blockHandle;
		MemoryRange bufferContentRange;
		MemoryRange allocationRange;

		VkBuffer buffers[frameInFlightCount];
		void* mappedData[frameInFlightCount];
	};

	struct ImageResourceViewInfo {
		VkImageViewCreateFlags flags;
		VkImageViewType viewType;
		VkComponentMapping components;
		VkImageSubresourceRange subresourceRange;
	};

	struct AllocationResult {
		MemoryRange allocationRange;
		MemoryRange usableRange;
		BlockHandle blockHandle;
	};

	inline bool operator==(const ImageResourceViewInfo& one, const ImageResourceViewInfo& other) {
		return one.flags == other.flags && one.viewType == other.viewType && one.components.r == other.components.r &&
			   one.components.g == other.components.g && one.components.b == other.components.b &&
			   one.components.a == other.components.a &&
			   one.subresourceRange.aspectMask == other.subresourceRange.aspectMask &&
			   one.subresourceRange.baseArrayLayer == other.subresourceRange.baseArrayLayer &&
			   one.subresourceRange.layerCount == other.subresourceRange.layerCount &&
			   one.subresourceRange.baseMipLevel == other.subresourceRange.baseMipLevel &&
			   one.subresourceRange.levelCount == other.subresourceRange.levelCount;
	}
} // namespace vanadium::graphics

namespace std {
	template <> struct hash<vanadium::graphics::ImageResourceViewInfo> {
		size_t operator()(const vanadium::graphics::ImageResourceViewInfo& info) const {
			uint32_t componentMappingID =
				info.components.r << 9 | info.components.g << 6 | info.components.b << 3 | info.components.a;
			size_t subresourceRangeHash = ((std::hash<VkImageAspectFlags>()(info.subresourceRange.aspectMask) ^
											std::hash<uint32_t>()(info.subresourceRange.baseArrayLayer)) +
										   (std::hash<uint32_t>()(info.subresourceRange.layerCount) ^
											std::hash<uint32_t>()(info.subresourceRange.baseMipLevel))) *
										  std::hash<uint32_t>()(info.subresourceRange.levelCount);
			return (std::hash<VkImageViewCreateFlags>()(info.flags) ^ std::hash<VkImageViewType>()(info.viewType)) +
				   (std::hash<uint32_t>()(componentMappingID) ^ subresourceRangeHash);
		}
	};
} // namespace std

namespace vanadium::graphics {

	struct ImageResourceInfo {
		VkFormat format;
		VkExtent3D dimensions;

		uint32_t mipLevelCount;
		uint32_t arrayLayerCount;
	};

	struct ImageAllocation {
		ImageResourceInfo resourceInfo;
		robin_hood::unordered_map<ImageResourceViewInfo, VkImageView> views;

		uint32_t typeIndex;
		BlockHandle blockHandle;
		VkDeviceSize alignmentMargin;
		MemoryRange allocationRange;
		VkImage image;
	};
	using BufferResourceHandle = SlotmapHandle;
	using ImageResourceHandle = SlotmapHandle;

	class GPUResourceAllocator {
	  public:
		GPUResourceAllocator() {}
		GPUResourceAllocator(const GPUResourceAllocator&) = delete;
		GPUResourceAllocator& operator=(const GPUResourceAllocator&) = delete;
		GPUResourceAllocator(GPUResourceAllocator&&) = default;
		GPUResourceAllocator& operator=(GPUResourceAllocator&&) = default;

		void create(DeviceContext* gpuContext);

		BlockHandle createBufferBlock(size_t size, MemoryCapabilities required, MemoryCapabilities preferred, bool createMapped);
		BlockHandle createImageBlock(size_t size, MemoryCapabilities required, MemoryCapabilities preferred);

		BufferResourceHandle createBuffer(const VkBufferCreateInfo& bufferCreateInfo, MemoryCapabilities required,
										  MemoryCapabilities preferred, bool createMapped);
		BufferResourceHandle createPerFrameBuffer(const VkBufferCreateInfo& bufferCreateInfo,
												  MemoryCapabilities required, MemoryCapabilities preferred,
												  bool createMapped);
		BufferResourceHandle createBuffer(const VkBufferCreateInfo& bufferCreateInfo, BlockHandle block, bool createMapped);

		MemoryCapabilities bufferMemoryCapabilities(BufferResourceHandle handle);
		VkDeviceMemory nativeMemoryHandle(BufferResourceHandle handle);
		MemoryRange allocationRange(BufferResourceHandle handle);
		VkBuffer nativeBufferHandle(BufferResourceHandle handle) {
			return m_buffers[handle].buffers[m_currentFrameIndex];
		}
		void* mappedBufferData(BufferResourceHandle handle);
		void destroyBuffer(BufferResourceHandle handle);

		ImageResourceHandle createImage(const VkImageCreateInfo& imageCreateInfo, MemoryCapabilities required,
										MemoryCapabilities preferred);
		ImageResourceHandle createImage(const VkImageCreateInfo& imageCreateInfo, BlockHandle block);
		VkImage nativeImageHandle(ImageResourceHandle handle);
		const ImageResourceInfo& imageResourceInfo(ImageResourceHandle handle);
		VkImageView requestImageView(ImageResourceHandle handle, const ImageResourceViewInfo& info);
		void destroyImage(ImageResourceHandle handle);

		void destroyBufferBlock(BlockHandle handle);
		void destroyImageBlock(BlockHandle handle);
		void destroy();

		void setFrameIndex(uint32_t frameIndex);
		void updateMemoryBudget();

	  private:
		static constexpr VkDeviceSize m_blockSize = 32_MiB;
		static constexpr VkDeviceSize m_bufferBlockFreeThreshold = 64_MiB;
		static constexpr VkDeviceSize m_imageBlockFreeThreshold = 128_MiB;

		void flushFreeList();

		uint32_t bestTypeIndex(VkMemoryPropertyFlags required, VkMemoryPropertyFlags preferred,
							   VkMemoryRequirements requirements, bool createMapped);
		bool isTypeBigEnough(uint32_t typeIndex, VkDeviceSize size, bool createMapped);
		std::optional<AllocationResult> allocate(uint32_t typeIndex, VkDeviceSize alignment, VkDeviceSize size,
												 bool createMapped);
		std::optional<AllocationResult> allocateImage(uint32_t typeIndex, VkDeviceSize alignment, VkDeviceSize size);
		std::optional<AllocationResult> allocateInBlock(uint32_t typeIndex, BlockHandle blockHandle, MemoryBlock& block,
														VkDeviceSize alignment, VkDeviceSize size, bool createMapped);
		std::optional<AllocationResult> allocateInBlock(BlockHandle blockHandle, MemoryBlock& block,
														VkDeviceSize alignment, VkDeviceSize size, bool createMapped);
		void freeInBlock(MemoryBlock& block, VkDeviceSize offset, VkDeviceSize size);
		void mergeFreeAreas(MemoryBlock& block);

		bool allocateBlock(uint32_t typeIndex, VkDeviceSize size, bool createMapped, bool createImageBlock);
		bool allocateCustomBlock(uint32_t typeIndex, VkDeviceSize size, bool createMapped, bool createImageBlock);

		void reorderOffsetArea(MemoryBlock& block, size_t index);
		void reorderSizeArea(MemoryBlock& block, size_t index);

		VkDeviceSize roundUpAligned(VkDeviceSize n, VkDeviceSize alignment);
		VkDeviceSize alignmentMargin(VkDeviceSize n, VkDeviceSize alignment);

		DeviceContext* m_context = nullptr;

		uint32_t m_currentFrameIndex = 0;

		VkDeviceSize m_bufferImageGranularity;

		std::vector<MemoryType> m_memoryTypes;
		std::vector<size_t> m_heapBudgets;

		Slotmap<MemoryBlock> m_customBufferBlocks;
		Slotmap<MemoryBlock> m_customImageBlocks;

		Slotmap<BufferAllocation> m_buffers;
		Slotmap<ImageAllocation> m_images;

		std::vector<std::vector<BufferAllocation>> m_bufferFreeList;
		std::vector<std::vector<ImageAllocation>> m_imageFreeList;
		std::vector<std::vector<MemoryBlock>> m_blockFreeList;
	};

} // namespace vanadium::graphics