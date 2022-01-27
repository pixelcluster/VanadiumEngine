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

using VBufferResourceHandle = VSlotmapHandle;

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

	void destroy();

  private:
	VGPUContext* m_context = nullptr;
	VmaAllocator m_allocator;

	std::vector<VkMemoryType> m_memoryTypes;

	VSlotmap<VBufferAllocation> m_buffers;
};