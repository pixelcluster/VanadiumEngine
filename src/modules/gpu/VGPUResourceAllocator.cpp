#include <modules/gpu/VGPUResourceAllocator.hpp>
#include <volk.h>
#include <modules/gpu/helper/ErrorHelper.hpp>

void VGPUResourceAllocator::create(VGPUContext* gpuContext) {
	VmaVulkanFunctions functions = { .vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties,
									 .vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties,
									 .vkAllocateMemory = vkAllocateMemory,
									 .vkFreeMemory = vkFreeMemory,
									 .vkMapMemory = vkMapMemory,
									 .vkUnmapMemory = vkUnmapMemory,
									 .vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges,
									 .vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges,
									 .vkBindBufferMemory = vkBindBufferMemory,
									 .vkBindImageMemory = vkBindImageMemory,
									 .vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements,
									 .vkGetImageMemoryRequirements = vkGetImageMemoryRequirements,
									 .vkCreateBuffer = vkCreateBuffer,
									 .vkDestroyBuffer = vkDestroyBuffer,
									 .vkCreateImage = vkCreateImage,
									 .vkCmdCopyBuffer = vkCmdCopyBuffer,
									 .vkGetPhysicalDeviceMemoryProperties2KHR =
										 vkGetPhysicalDeviceMemoryProperties2KHR };

	VmaAllocatorCreateInfo allocatorCreateInfo = { .physicalDevice = gpuContext->physicalDevice(),
												   .device = gpuContext->device(),
												   .pVulkanFunctions = &functions,
												   .instance = gpuContext->instance(),
												   .vulkanApiVersion = VK_API_VERSION_1_0 };

	if (gpuContext->gpuCapabilities().memoryBudget) {
		allocatorCreateInfo.flags |= VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
	}
	if (gpuContext->gpuCapabilities().memoryPriority) {
		allocatorCreateInfo.flags |= VMA_ALLOCATOR_CREATE_EXT_MEMORY_PRIORITY_BIT;
	}

	verifyResult(vmaCreateAllocator(&allocatorCreateInfo, &m_allocator));
}

VBufferResourceHandle VGPUResourceAllocator::createBuffer(const VkBufferCreateInfo& bufferCreateInfo,
														 VmaMemoryUsage usage, float priority, bool createMapped) {
	VBufferAllocation allocation;
	VmaAllocationCreateInfo allocationCreateInfo = {
		.flags = createMapped ? VMA_ALLOCATION_CREATE_MAPPED_BIT : 0,
		.usage = usage,
		.priority = priority
	};
	VmaAllocationInfo allocationInfo;
	verifyResult(vmaCreateBuffer(m_allocator, &bufferCreateInfo, &allocationCreateInfo, &allocation.buffer, &allocation.allocation,
					&allocationInfo));
	allocation.mappedData = allocationInfo.pMappedData;

	return m_buffers.addElement(allocation);
}

void VGPUResourceAllocator::destroy() { vmaDestroyAllocator(m_allocator); }
