#pragma once

#include <helper/VSlotmap.hpp>
#include <modules/gpu/VGPUContext.hpp>
#include <array>
#include <vk_mem_alloc.h>

struct V {

};

struct VBufferAllocation {
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

	VSlotmap<VBufferAllocation> m_buffers;
};