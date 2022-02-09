#pragma once

#include <array>
#include <helper/VSlotmap.hpp>
#include <modules/gpu/VGPUContext.hpp>
#include <helper/VMemoryLiterals.hpp>

struct VMemoryCapabilities {
	bool deviceLocal;
	bool hostVisible;
	bool hostCoherent;
};

struct VMemoryRange {
	VkDeviceSize offset;
	VkDeviceSize size;
};

struct VMemoryBlock {
	std::vector<VMemoryRange> freeBlocksSizeSorted;
	std::vector<VMemoryRange> freeBlocksOffsetSorted;

	VMemoryCapabilities capabilities;

	VkDeviceSize maxAllocatableSize;
	VkDeviceSize originalSize;

	VkDeviceMemory memoryHandle;
	void* mappedPointer;
};

struct VMemoryType {
	VkMemoryPropertyFlags properties;
	uint32_t heapIndex;

	std::vector<VMemoryBlock> blocks;
	std::vector<VMemoryBlock> imageBlocks;
};

struct VBufferAllocation {
	bool isMultipleBuffered = false;

	uint32_t typeIndex;
	size_t blockIndex;
	VMemoryRange bufferContentRange;
	VMemoryRange allocationRange;

	VkBuffer buffers[frameInFlightCount];
	void* mappedData[frameInFlightCount];
};

struct VImageResourceViewInfo {
	VkImageViewCreateFlags flags;
	VkImageViewType viewType;
	VkComponentMapping components;
	VkImageSubresourceRange subresourceRange;
};

struct VAllocationResult {
	VMemoryRange allocationRange;
	VMemoryRange usableRange;
	size_t blockIndex;
};

inline bool operator==(const VImageResourceViewInfo& one, const VImageResourceViewInfo& other) {
	return one.flags == other.flags && one.viewType == other.viewType && one.components.r == other.components.r &&
		   one.components.g == other.components.g && one.components.b == other.components.b &&
		   one.components.a == other.components.a &&
		   one.subresourceRange.aspectMask == other.subresourceRange.aspectMask &&
		   one.subresourceRange.baseArrayLayer == other.subresourceRange.baseArrayLayer &&
		   one.subresourceRange.layerCount == other.subresourceRange.layerCount &&
		   one.subresourceRange.baseMipLevel == other.subresourceRange.baseMipLevel &&
		   one.subresourceRange.levelCount == other.subresourceRange.levelCount;
}

namespace std {
	template <> struct hash<VImageResourceViewInfo> {
		size_t operator()(const VImageResourceViewInfo& info) const {
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

struct VImageResourceInfo {
	VkFormat format;
	VkExtent3D dimensions;

	uint32_t mipLevelCount;
	uint32_t arrayLayerCount;
};

struct VImageAllocation {
	VImageResourceInfo resourceInfo;
	std::unordered_map<VImageResourceViewInfo, VkImageView> views;

	uint32_t typeIndex;
	size_t blockIndex;
	VkDeviceSize alignmentMargin;
	VMemoryRange allocationRange;
	VkImage image;
};
using VBufferResourceHandle = VSlotmapHandle;
using VImageResourceHandle = VSlotmapHandle;

class VGPUResourceAllocator {
  public:
	VGPUResourceAllocator() {}
	VGPUResourceAllocator(const VGPUResourceAllocator&) = delete;
	VGPUResourceAllocator& operator=(const VGPUResourceAllocator&) = delete;
	VGPUResourceAllocator(VGPUResourceAllocator&&) = default;
	VGPUResourceAllocator& operator=(VGPUResourceAllocator&&) = default;

	void create(VGPUContext* gpuContext);

	VBufferResourceHandle createBuffer(const VkBufferCreateInfo& bufferCreateInfo, VMemoryCapabilities required,
									   VMemoryCapabilities preferred, bool createMapped);
	VBufferResourceHandle createPerFrameBuffer(const VkBufferCreateInfo& bufferCreateInfo,
													   VMemoryCapabilities required, VMemoryCapabilities preferred, bool createMapped);

	VMemoryCapabilities bufferMemoryCapabilities(VBufferResourceHandle handle);
	VkDeviceMemory nativeMemoryHandle(VBufferResourceHandle handle);
	VMemoryRange allocationRange(VBufferResourceHandle handle);
	VkBuffer nativeBufferHandle(VBufferResourceHandle handle) { return m_buffers[handle].buffers[m_currentFrameIndex]; }
	void* mappedBufferData(VBufferResourceHandle handle);
	void destroyBuffer(VBufferResourceHandle handle);

	VImageResourceHandle createImage(const VkImageCreateInfo& imageCreateInfo, VMemoryCapabilities required,
									 VMemoryCapabilities preferred);
	VkImage nativeImageHandle(VImageResourceHandle handle);
	const VImageResourceInfo& imageResourceInfo(VImageResourceHandle handle);
	VkImageView requestImageView(VImageResourceHandle handle, const VImageResourceViewInfo& info);
	void destroyImage(VImageResourceHandle handle);

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
	std::optional<VAllocationResult> allocate(uint32_t typeIndex, VkDeviceSize alignment, VkDeviceSize size,
											  bool createMapped);
	std::optional<VAllocationResult> allocateImage(uint32_t typeIndex, VkDeviceSize alignment, VkDeviceSize size);
	std::optional<VAllocationResult> allocateInBlock(uint32_t typeIndex, size_t blockIndex, VMemoryBlock& block, VkDeviceSize alignment, VkDeviceSize size,
											  bool createMapped);
	//returns true if whole block is free
	bool freeInBlock(VMemoryBlock& block, VkDeviceSize offset,
					 VkDeviceSize size);
	void mergeFreeAreas(VMemoryBlock& block);
	bool allocateBlock(uint32_t typeIndex, VkDeviceSize size, bool createMapped, bool createImageBlock);

	VkDeviceSize roundUpAligned(VkDeviceSize n, VkDeviceSize alignment);
	VkDeviceSize alignmentMargin(VkDeviceSize n, VkDeviceSize alignment);

	VGPUContext* m_context = nullptr;

	uint32_t m_currentFrameIndex = 0;

	VkDeviceSize m_bufferImageGranularity;

	std::vector<VMemoryType> m_memoryTypes;

	std::vector<size_t> m_heapBudgets;

	VSlotmap<VBufferAllocation> m_buffers;
	VSlotmap<VImageAllocation> m_images;

	std::vector<std::vector<VBufferAllocation>> m_bufferFreeList;
	std::vector<std::vector<VImageAllocation>> m_imageFreeList;
};