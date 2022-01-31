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

	VkPhysicalDeviceMemoryProperties memoryProperties = {};
	vkGetPhysicalDeviceMemoryProperties(gpuContext->physicalDevice(), &memoryProperties);

	m_memoryTypes = std::vector<VkMemoryType>(memoryProperties.memoryTypeCount);
	std::memcpy(m_memoryTypes.data(), memoryProperties.memoryTypes,
				memoryProperties.memoryTypeCount * sizeof(VkMemoryType));
}

VBufferResourceHandle VGPUResourceAllocator::createBuffer(const VkBufferCreateInfo& bufferCreateInfo,
														 VmaMemoryUsage usage, float priority, bool createMapped) {
	VBufferAllocation allocation;
	VmaAllocationCreateInfo allocationCreateInfo = {
		.flags = createMapped ? VMA_ALLOCATION_CREATE_MAPPED_BIT : 0U,
		.usage = usage,
		.priority = priority
	};
	VmaAllocationInfo allocationInfo;
	verifyResult(vmaCreateBuffer(m_allocator, &bufferCreateInfo, &allocationCreateInfo, &allocation.buffer, &allocation.allocation,
					&allocationInfo));
	allocation.mappedData = allocationInfo.pMappedData;

	allocation.capabilities.deviceLocal =
		m_memoryTypes[allocationInfo.memoryType].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	allocation.capabilities.hostVisible =
		m_memoryTypes[allocationInfo.memoryType].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
	allocation.capabilities.hostCoherent =
		m_memoryTypes[allocationInfo.memoryType].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

	return m_buffers.addElement(allocation);
}

void* VGPUResourceAllocator::mappedBufferData(VBufferResourceHandle handle) { return m_buffers[handle].mappedData; }

void VGPUResourceAllocator::destroyBuffer(VBufferResourceHandle handle) {
	vmaDestroyBuffer(m_allocator, m_buffers[handle].buffer, m_buffers[handle].allocation);
	m_buffers.removeElement(handle);
}

VImageResourceHandle VGPUResourceAllocator::createExternalImage(VkImage image) {
	return m_images.addElement(VImageAllocation { .image = image });
}

void VGPUResourceAllocator::updateExternalImage(VImageResourceHandle handle, VkImage image) {
	auto iterator = m_images.find(handle);

	if (iterator == m_images.end()) {
		//TODO: log trying to update an external image that wasn't external
	} else {
		iterator->image = image;
	}
}

VkImage VGPUResourceAllocator::nativeImageHandle(VImageResourceHandle handle) { return m_images[handle].image; }

const VImageResourceInfo& VGPUResourceAllocator::imageResourceInfo(VImageResourceHandle handle) {
	return m_images[handle].resourceInfo;
}

void VGPUResourceAllocator::destroy() { vmaDestroyAllocator(m_allocator); }
