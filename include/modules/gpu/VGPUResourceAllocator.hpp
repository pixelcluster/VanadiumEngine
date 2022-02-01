#pragma once

#include <helper/VSlotmap.hpp>
#include <modules/gpu/VGPUContext.hpp>
#include <array>
#include <vk_mem_alloc.h>

struct VBufferMemoryCapabilities {
	bool deviceLocal;
	bool hostVisible;
	bool hostCoherent;
};

struct VBufferAllocation {
	VBufferMemoryCapabilities capabilities;

	VmaAllocation allocation;
	VkBuffer buffer;
	void* mappedData;
};

struct VImageResourceViewInfo {
	VkImageViewCreateFlags flags;
	VkImageViewType viewType;
	VkComponentMapping components;
	VkImageSubresourceRange subresourceRange;
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
}

struct VImageResourceInfo {
	VkFormat format;
	VkExtent3D dimensions;

	uint32_t mipLevelCount;
	uint32_t arrayLayerCount;
};

struct VImageAllocation {
	VImageResourceInfo resourceInfo;
	std::unordered_map<VImageResourceViewInfo, VkImageView> views;

	VmaAllocation allocation = nullptr;
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

	VBufferResourceHandle createBuffer(const VkBufferCreateInfo& bufferCreateInfo, VmaMemoryUsage usage, float priority, bool createMapped);
	VkBuffer nativeBufferHandle(VBufferResourceHandle handle) { return m_buffers[handle].buffer; }
	void* mappedBufferData(VBufferResourceHandle handle);
	void destroyBuffer(VBufferResourceHandle handle);

	VkImage nativeImageHandle(VImageResourceHandle handle);
	const VImageResourceInfo& imageResourceInfo(VImageResourceHandle handle);
	VkImageView requestImageView(VImageResourceHandle handle, const VImageResourceViewInfo& info);

	void destroy();

  private:
	VGPUContext* m_context = nullptr;
	VmaAllocator m_allocator;

	std::vector<VkMemoryType> m_memoryTypes;

	VSlotmap<VBufferAllocation> m_buffers;
	VSlotmap<VImageAllocation> m_images;
};