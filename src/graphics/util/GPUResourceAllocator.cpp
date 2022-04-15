#include <bit>
#include <graphics/helper/ErrorHelper.hpp>
#include <graphics/util/GPUResourceAllocator.hpp>
#include <helper/SharedLockGuard.hpp>
#include <volk.h>

namespace vanadium::graphics {

	void GPUResourceAllocator::create(DeviceContext* gpuContext) {
		m_bufferFreeList.resize(frameInFlightCount);
		m_imageFreeList.resize(frameInFlightCount);
		m_blockFreeList.resize(frameInFlightCount);

		if (gpuContext->deviceCapabilities().memoryBudget) {
			VkPhysicalDeviceMemoryProperties2KHR memoryProperties2 = {
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2_KHR
			};
			VkPhysicalDeviceMemoryBudgetPropertiesEXT memoryBudgetProperties = {
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT
			};
			memoryProperties2.pNext = &memoryBudgetProperties;

			vkGetPhysicalDeviceMemoryProperties2KHR(gpuContext->physicalDevice(), &memoryProperties2);
			m_memoryTypes.resize(memoryProperties2.memoryProperties.memoryTypeCount);
			m_heapBudgets.resize(memoryProperties2.memoryProperties.memoryHeapCount);

			for (uint32_t i = 0; i < memoryProperties2.memoryProperties.memoryHeapCount; ++i) {
				m_heapBudgets[i] = memoryBudgetProperties.heapBudget[i];
			}
			for (uint32_t i = 0; i < memoryProperties2.memoryProperties.memoryTypeCount; ++i) {
				m_memoryTypes[i].properties = memoryProperties2.memoryProperties.memoryTypes[i].propertyFlags;
				m_memoryTypes[i].heapIndex = memoryProperties2.memoryProperties.memoryTypes[i].heapIndex;
			}

		} else {
			VkPhysicalDeviceMemoryProperties memoryProperties = {};
			vkGetPhysicalDeviceMemoryProperties(gpuContext->physicalDevice(), &memoryProperties);
			m_memoryTypes.resize(memoryProperties.memoryTypeCount);
			m_heapBudgets.resize(memoryProperties.memoryHeapCount);

			for (uint32_t i = 0; i < memoryProperties.memoryHeapCount; ++i) {
				m_heapBudgets[i] = memoryProperties.memoryHeaps[i].size;
			}
			for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i) {
				m_memoryTypes[i].properties = memoryProperties.memoryTypes[i].propertyFlags;
				m_memoryTypes[i].heapIndex = memoryProperties.memoryTypes[i].heapIndex;
			}
		}

		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(gpuContext->physicalDevice(), &properties);
		m_bufferImageGranularity = properties.limits.bufferImageGranularity;

		m_context = gpuContext;
	}

	BlockHandle GPUResourceAllocator::createBufferBlock(size_t size, MemoryCapabilities required,
														MemoryCapabilities preferred, bool createMapped) {
		auto lock = std::lock_guard<std::shared_mutex>(m_accessMutex);
		VkMemoryPropertyFlags requiredFlags = (required.deviceLocal ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT : 0) |
											  (required.hostCoherent ? VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : 0) |
											  ((required.hostVisible) ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT : 0);
		VkMemoryPropertyFlags preferredFlags = (preferred.deviceLocal ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT : 0) |
											   (preferred.hostCoherent ? VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : 0) |
											   (preferred.hostVisible ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT : 0);

		uint32_t typeIndex =
			bestTypeIndex(requiredFlags, preferredFlags, { .size = 0, .memoryTypeBits = 0xFFFFFF }, false);
		return allocateCustomBlock(typeIndex, size, createMapped, false);
	}

	BlockHandle GPUResourceAllocator::createImageBlock(size_t size, MemoryCapabilities required,
													   MemoryCapabilities preferred) {
		auto lock = std::lock_guard<std::shared_mutex>(m_accessMutex);
		VkMemoryPropertyFlags requiredFlags = (required.deviceLocal ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT : 0) |
											  (required.hostCoherent ? VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : 0) |
											  ((required.hostVisible) ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT : 0);
		VkMemoryPropertyFlags preferredFlags = (preferred.deviceLocal ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT : 0) |
											   (preferred.hostCoherent ? VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : 0) |
											   (preferred.hostVisible ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT : 0);

		uint32_t typeIndex =
			bestTypeIndex(requiredFlags, preferredFlags, { .size = 0, .memoryTypeBits = 0xFFFFFF }, false);
		return allocateCustomBlock(typeIndex, size, false, true);
	}

	BufferResourceHandle GPUResourceAllocator::createBuffer(const VkBufferCreateInfo& bufferCreateInfo,
															MemoryCapabilities required, MemoryCapabilities preferred,
															bool createMapped) {
		auto lock = std::lock_guard<std::shared_mutex>(m_accessMutex);
		VkMemoryPropertyFlags requiredFlags = (required.deviceLocal ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT : 0) |
											  (required.hostCoherent ? VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : 0) |
											  (required.hostVisible ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT : 0);
		VkMemoryPropertyFlags preferredFlags = (preferred.deviceLocal ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT : 0) |
											   (preferred.hostCoherent ? VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : 0) |
											   (preferred.hostVisible ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT : 0);

		VkBuffer buffer;
		verifyResult(vkCreateBuffer(m_context->device(), &bufferCreateInfo, nullptr, &buffer));

		VkMemoryRequirements requirements;
		vkGetBufferMemoryRequirements(m_context->device(), buffer, &requirements);

		uint32_t typeIndex = bestTypeIndex(requiredFlags, preferredFlags, requirements, createMapped);

		if (typeIndex == ~0U) {
			vkDestroyBuffer(m_context->device(), buffer, nullptr);
			return ~0U;
		}

		auto result = allocate(typeIndex, requirements.alignment, requirements.size, createMapped);
		if (!result.has_value()) {
			typeIndex = 0;
			for (auto& type : m_memoryTypes) {
				if ((type.properties & requiredFlags) != requiredFlags) {
					++typeIndex;
					continue;
				}
				result = allocate(typeIndex, requirements.alignment, requirements.size, createMapped);
				if (result.has_value())
					break;
				++typeIndex;
			}
			--typeIndex;
			if (!result.has_value()) {
				vkDestroyBuffer(m_context->device(), buffer, nullptr);
				return ~0U;
			}
		}

		BufferAllocation allocation = { .isMultipleBuffered = false,
										.typeIndex = typeIndex,
										.blockHandle = result.value().blockHandle,
										.bufferContentRange = result.value().usableRange,
										.allocationRange = result.value().allocationRange };
		verifyResult(vkBindBufferMemory(m_context->device(), buffer,
										m_memoryTypes[typeIndex].blocks[result.value().blockHandle].memoryHandle,
										result.value().usableRange.offset));
		for (size_t i = 0; i < frameInFlightCount; ++i) {
			allocation.buffers[i] = buffer;
			if (createMapped) {
				auto bufferStartPointer =
					reinterpret_cast<uintptr_t>(
						m_memoryTypes[typeIndex].blocks[result.value().blockHandle].mappedPointer) +
					result.value().usableRange.offset;
				allocation.mappedData[i] = reinterpret_cast<void*>(bufferStartPointer);
			}
		}

		return m_buffers.addElement(allocation);
	}

	BufferResourceHandle GPUResourceAllocator::createPerFrameBuffer(const VkBufferCreateInfo& bufferCreateInfo,
																	MemoryCapabilities required,
																	MemoryCapabilities preferred, bool createMapped) {
		auto lock = std::lock_guard<std::shared_mutex>(m_accessMutex);
		VkMemoryPropertyFlags requiredFlags = (required.deviceLocal ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT : 0) |
											  (required.hostCoherent ? VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : 0) |
											  (required.hostVisible ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT : 0);
		VkMemoryPropertyFlags preferredFlags = (preferred.deviceLocal ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT : 0) |
											   (preferred.hostCoherent ? VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : 0) |
											   (preferred.hostVisible ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT : 0);

		VkBuffer buffer;
		verifyResult(vkCreateBuffer(m_context->device(), &bufferCreateInfo, nullptr, &buffer));

		VkMemoryRequirements requirements;
		vkGetBufferMemoryRequirements(m_context->device(), buffer, &requirements);

		VkDeviceSize alignedSize = roundUpAligned(requirements.size, requirements.alignment);
		VkDeviceSize totalSize = (frameInFlightCount - 1) * alignedSize + requirements.size;

		requirements.size = totalSize;

		uint32_t typeIndex = bestTypeIndex(requiredFlags, preferredFlags, requirements, createMapped);

		if (typeIndex == ~0U) {
			vkDestroyBuffer(m_context->device(), buffer, nullptr);
			return ~0U;
		}

		auto result = allocate(typeIndex, requirements.alignment, totalSize, createMapped);
		if (!result.has_value()) {
			typeIndex = 0;
			for (auto& type : m_memoryTypes) {
				if ((type.properties & requiredFlags) != requiredFlags) {
					++typeIndex;
					continue;
				}
				result = allocate(typeIndex, requirements.alignment, totalSize, createMapped);
				if (result.has_value())
					break;
				++typeIndex;
			}
			--typeIndex;
			if (!result.has_value()) {
				vkDestroyBuffer(m_context->device(), buffer, nullptr);
				return ~0U;
			}
		}

		BufferAllocation allocation = { .isMultipleBuffered = true,
										.typeIndex = typeIndex,
										.blockHandle = result.value().blockHandle,
										.bufferContentRange = result.value().usableRange,
										.allocationRange = result.value().allocationRange };
		allocation.buffers[0] = buffer;
		for (size_t i = 1; i < frameInFlightCount; ++i) {
			verifyResult(vkCreateBuffer(m_context->device(), &bufferCreateInfo, nullptr, &allocation.buffers[i]));
		}
		for (size_t i = 0; i < frameInFlightCount; ++i) {
			verifyResult(vkBindBufferMemory(m_context->device(), allocation.buffers[i],
											m_memoryTypes[typeIndex].blocks[result.value().blockHandle].memoryHandle,
											result.value().usableRange.offset + i * alignedSize));
			if (createMapped) {
				auto bufferStartPointer =
					reinterpret_cast<uintptr_t>(
						m_memoryTypes[typeIndex].blocks[result.value().blockHandle].mappedPointer) +
					result.value().usableRange.offset + i * alignedSize;
				allocation.mappedData[i] = reinterpret_cast<void*>(bufferStartPointer);
			}
		}

		return m_buffers.addElement(allocation);
	}

	BufferResourceHandle GPUResourceAllocator::createBuffer(const VkBufferCreateInfo& bufferCreateInfo,
															BlockHandle block, bool createMapped) {
		auto lock = std::lock_guard<std::shared_mutex>(m_accessMutex);
		if (createMapped && !m_customBufferBlocks[block].mappedPointer) {
			return ~0U;
		}

		VkBuffer buffer;
		verifyResult(vkCreateBuffer(m_context->device(), &bufferCreateInfo, nullptr, &buffer));

		VkMemoryRequirements requirements;
		vkGetBufferMemoryRequirements(m_context->device(), buffer, &requirements);

		auto result = allocateInBlock(block, m_customBufferBlocks[block], requirements.alignment, requirements.size,
									  createMapped);
		if (!result.has_value()) {
			return ~0U;
		} else {
			BufferAllocation allocation = { .isMultipleBuffered = false,
											.typeIndex = ~0U,
											.blockHandle = result.value().blockHandle,
											.bufferContentRange = result.value().usableRange,
											.allocationRange = result.value().allocationRange };
			verifyResult(vkBindBufferMemory(m_context->device(), buffer, m_customBufferBlocks[block].memoryHandle,
											result.value().usableRange.offset));
			for (size_t i = 0; i < frameInFlightCount; ++i) {
				allocation.buffers[i] = buffer;
				if (createMapped) {
					auto bufferStartPointer = reinterpret_cast<uintptr_t>(m_customBufferBlocks[block].mappedPointer) +
											  result.value().usableRange.offset;
					allocation.mappedData[i] = reinterpret_cast<void*>(bufferStartPointer);
				}
			}
			return m_buffers.addElement(allocation);
		}
	}

	MemoryCapabilities GPUResourceAllocator::bufferMemoryCapabilities(BufferResourceHandle handle) {
		auto lock = SharedLockGuard(m_accessMutex);
		return m_memoryTypes[m_buffers[handle].typeIndex].blocks[m_buffers[handle].blockHandle].capabilities;
	}

	VkDeviceMemory GPUResourceAllocator::nativeMemoryHandle(BufferResourceHandle handle) {
		auto lock = SharedLockGuard(m_accessMutex);
		return m_memoryTypes[m_buffers[handle].typeIndex].blocks[m_buffers[handle].blockHandle].memoryHandle;
	}

	MemoryRange GPUResourceAllocator::allocationRange(BufferResourceHandle handle) {
		auto lock = SharedLockGuard(m_accessMutex);
		return m_buffers[handle].bufferContentRange;
	}

	VkBuffer GPUResourceAllocator::nativeBufferHandle(BufferResourceHandle handle) {
		auto lock = SharedLockGuard(m_accessMutex);
		return m_buffers[handle].buffers[m_currentFrameIndex];
	}

	void* GPUResourceAllocator::mappedBufferData(BufferResourceHandle handle) {
		auto lock = SharedLockGuard(m_accessMutex);
		return m_buffers[handle].mappedData[m_currentFrameIndex];
	}

	void GPUResourceAllocator::destroyBuffer(BufferResourceHandle handle) {
		auto lock = std::lock_guard<std::shared_mutex>(m_accessMutex);
		auto& allocation = m_buffers[handle];
		m_bufferFreeList[m_currentFrameIndex].push_back(allocation);
		m_buffers.removeElement(handle);
	}

	void GPUResourceAllocator::destroyBufferImmediately(BufferResourceHandle handle) {
		auto lock = std::lock_guard<std::shared_mutex>(m_accessMutex);
		auto& allocation = m_buffers[handle];
		destroyBufferImmediatelyUnsynchronized(allocation);
		m_buffers.removeElement(handle);
	}

	void GPUResourceAllocator::destroyBufferImmediatelyUnsynchronized(const BufferAllocation& allocation) {
		if (allocation.isMultipleBuffered) {
			for (auto& buffer : allocation.buffers) {
				vkDestroyBuffer(m_context->device(), buffer, nullptr);
			}
		} else {
			vkDestroyBuffer(m_context->device(), allocation.buffers[0], nullptr);
		}

		if (allocation.typeIndex != ~0U) {
			freeInBlock(m_memoryTypes[allocation.typeIndex].blocks[allocation.blockHandle],
						allocation.allocationRange.offset, allocation.allocationRange.size);
		} else {
			freeInBlock(m_customBufferBlocks[allocation.blockHandle], allocation.allocationRange.offset,
						allocation.allocationRange.size);
		}
	}

	ImageResourceHandle GPUResourceAllocator::createImage(const VkImageCreateInfo& imageCreateInfo,
														  MemoryCapabilities required, MemoryCapabilities preferred) {
		auto lock = std::lock_guard<std::shared_mutex>(m_accessMutex);
		// clang-format off
	VkMemoryPropertyFlags requiredFlags = (required.deviceLocal  ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT  : 0) | 
										  (required.hostCoherent ? VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : 0) |
										  (required.hostVisible  ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT  : 0);
	VkMemoryPropertyFlags preferredFlags = (preferred.deviceLocal  ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT  : 0) | 
										   (preferred.hostCoherent ? VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : 0) |
										   (preferred.hostVisible  ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT  : 0);
		// clang-format on

		VkImage image;
		verifyResult(vkCreateImage(m_context->device(), &imageCreateInfo, nullptr, &image));

		VkMemoryRequirements requirements;
		vkGetImageMemoryRequirements(m_context->device(), image, &requirements);

		uint32_t typeIndex = bestTypeIndex(requiredFlags, preferredFlags, requirements, false);

		if (typeIndex == ~0U) {
			vkDestroyImage(m_context->device(), image, nullptr);
			return ~0U;
		}

		auto result = allocateImage(typeIndex, requirements.alignment, requirements.size);
		if (!result.has_value()) {
			typeIndex = 0;
			for (auto& type : m_memoryTypes) {
				if ((type.properties & requiredFlags) != requiredFlags) {
					++typeIndex;
					continue;
				}
				result = allocateImage(typeIndex, requirements.alignment, requirements.size);
				if (result.has_value())
					break;
				++typeIndex;
			}
			--typeIndex;
			if (!result.has_value()) {
				vkDestroyImage(m_context->device(), image, nullptr);
				return ~0U;
			}
		}

		ImageAllocation allocation = {
			.resourceInfo = { .format = imageCreateInfo.format,
							  .dimensions = imageCreateInfo.extent,
							  .mipLevelCount = imageCreateInfo.mipLevels,
							  .arrayLayerCount = imageCreateInfo.arrayLayers },
			.typeIndex = typeIndex,
			.blockHandle = result.value().blockHandle,
			.allocationRange = result.value().allocationRange,
		};
		allocation.image = image;
		vkBindImageMemory(m_context->device(), image,
						  m_memoryTypes[typeIndex].imageBlocks[result.value().blockHandle].memoryHandle,
						  result.value().usableRange.offset);
		return m_images.addElement(allocation);
	}

	ImageResourceHandle GPUResourceAllocator::createImage(const VkImageCreateInfo& imageCreateInfo, BlockHandle block) {
		auto lock = std::lock_guard<std::shared_mutex>(m_accessMutex);
		VkImage image;
		verifyResult(vkCreateImage(m_context->device(), &imageCreateInfo, nullptr, &image));

		VkMemoryRequirements requirements;
		vkGetImageMemoryRequirements(m_context->device(), image, &requirements);

		auto result =
			allocateInBlock(block, m_customImageBlocks[block], requirements.alignment, requirements.size, false);
		if (!result.has_value()) {
			return ~0U;
		} else {
			ImageAllocation allocation = {
				.resourceInfo = { .format = imageCreateInfo.format,
								  .dimensions = imageCreateInfo.extent,
								  .mipLevelCount = imageCreateInfo.mipLevels,
								  .arrayLayerCount = imageCreateInfo.arrayLayers },
				.typeIndex = ~0U,
				.blockHandle = result.value().blockHandle,
				.allocationRange = result.value().allocationRange,
			};
			allocation.image = image;
			vkBindImageMemory(m_context->device(), image, m_customImageBlocks[block].memoryHandle,
							  result.value().usableRange.offset);
			return m_images.addElement(allocation);
		}
	}

	VkImage GPUResourceAllocator::nativeImageHandle(ImageResourceHandle handle) {
		auto lock = SharedLockGuard(m_accessMutex);
		return m_images[handle].image;
	}

	const ImageResourceInfo& GPUResourceAllocator::imageResourceInfo(ImageResourceHandle handle) {
		auto lock = SharedLockGuard(m_accessMutex);
		return m_images[handle].resourceInfo;
	}

	VkImageView GPUResourceAllocator::requestImageView(ImageResourceHandle handle, const ImageResourceViewInfo& info) {
		auto lock = std::lock_guard<std::shared_mutex>(m_accessMutex);
		auto viewIterator = m_images[handle].views.find(info);
		if (viewIterator == m_images[handle].views.end()) {
			VkImageViewCreateInfo createInfo = { .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
												 .flags = info.flags,
												 .image = m_images[handle].image,
												 .viewType = info.viewType,
												 .format = m_images[handle].resourceInfo.format,
												 .components = info.components,
												 .subresourceRange = info.subresourceRange };
			VkImageView view;
			verifyResult(vkCreateImageView(m_context->device(), &createInfo, nullptr, &view));

			m_images[handle].views.insert(robin_hood::pair<const ImageResourceViewInfo, VkImageView>(info, view));
			return view;
		} else
			return viewIterator->second;
	}

	void GPUResourceAllocator::destroyImage(ImageResourceHandle handle) {
		auto lock = std::lock_guard<std::shared_mutex>(m_accessMutex);
		auto& allocation = m_images[handle];
		m_imageFreeList[m_currentFrameIndex].push_back(allocation);
		m_images.removeElement(handle);
	}

	void GPUResourceAllocator::destroyImageImmediately(ImageResourceHandle handle) {
		auto lock = std::lock_guard<std::shared_mutex>(m_accessMutex);
		destroyImageImmediatelyUnsynchronized(m_images[handle]);
		m_images.removeElement(handle);
	}

	void GPUResourceAllocator::destroyImageImmediatelyUnsynchronized(const ImageAllocation& allocation) {
		for (auto& view : allocation.views) {
			vkDestroyImageView(m_context->device(), view.second, nullptr);
		}
		vkDestroyImage(m_context->device(), allocation.image, nullptr);

		if (allocation.typeIndex != ~0U) {
			freeInBlock(m_memoryTypes[allocation.typeIndex].imageBlocks[allocation.blockHandle],
						allocation.allocationRange.offset - allocation.alignmentMargin,
						allocation.allocationRange.size + allocation.alignmentMargin);
		} else {
			freeInBlock(m_customImageBlocks[allocation.blockHandle], allocation.allocationRange.offset,
						allocation.allocationRange.size);
		}
	}

	void GPUResourceAllocator::destroyBufferBlock(BlockHandle handle) {
		auto lock = std::lock_guard<std::shared_mutex>(m_accessMutex);
		m_blockFreeList[m_currentFrameIndex].push_back(m_customBufferBlocks[handle]);
		m_customBufferBlocks.removeElement(handle);
	}
	void GPUResourceAllocator::destroyImageBlock(BlockHandle handle) {
		auto lock = std::lock_guard<std::shared_mutex>(m_accessMutex);
		m_blockFreeList[m_currentFrameIndex].push_back(m_customImageBlocks[handle]);
		m_customImageBlocks.removeElement(handle);
	}

	void GPUResourceAllocator::destroy() {
		// work around iterator invalidation
		while (m_buffers.size()) {
			destroyBuffer(m_buffers.handle(m_buffers.begin()));
		}
		while (m_images.size()) {
			destroyImage(m_images.handle(m_images.begin()));
		}

		for (uint32_t i = 0; i < frameInFlightCount; ++i) {
			m_currentFrameIndex = i;
			flushFreeList();
		}

		for (auto& type : m_memoryTypes) {
			for (auto& block : type.blocks) {
				vkFreeMemory(m_context->device(), block.memoryHandle, nullptr);
			}
			for (auto& block : type.imageBlocks) {
				vkFreeMemory(m_context->device(), block.memoryHandle, nullptr);
			}
		}

		for (auto& block : m_customBufferBlocks) {
			vkFreeMemory(m_context->device(), block.memoryHandle, nullptr);
		}
		for (auto& block : m_customImageBlocks) {
			vkFreeMemory(m_context->device(), block.memoryHandle, nullptr);
		}
	}

	void GPUResourceAllocator::setFrameIndex(uint32_t frameIndex) {
		auto lock = std::lock_guard<std::shared_mutex>(m_accessMutex);
		m_currentFrameIndex = frameIndex;
		flushFreeList();
	}

	void GPUResourceAllocator::updateMemoryBudget() {
		auto lock = std::lock_guard<std::shared_mutex>(m_accessMutex);
		if (m_context->deviceCapabilities().memoryBudget) {
			VkPhysicalDeviceMemoryProperties2KHR memoryProperties2 = {
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2_KHR
			};
			VkPhysicalDeviceMemoryBudgetPropertiesEXT memoryBudgetProperties = {
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT
			};
			memoryProperties2.pNext = &memoryBudgetProperties;
			vkGetPhysicalDeviceMemoryProperties2KHR(m_context->physicalDevice(), &memoryProperties2);

			for (uint32_t i = 0; i < memoryProperties2.memoryProperties.memoryHeapCount; ++i) {
				m_heapBudgets[i] = memoryBudgetProperties.heapBudget[i];
			}
		}
	}

	void GPUResourceAllocator::flushFreeList() {
		for (auto& allocation : m_bufferFreeList[m_currentFrameIndex]) {
			destroyBufferImmediatelyUnsynchronized(allocation);
		}
		m_bufferFreeList[m_currentFrameIndex].clear();

		for (auto& allocation : m_imageFreeList[m_currentFrameIndex]) {
			destroyImageImmediatelyUnsynchronized(allocation);
		}
		m_imageFreeList[m_currentFrameIndex].clear();

		for (auto& type : m_memoryTypes) {
			size_t freeBlockCount = 0;

		blockFreeStart:
			auto iterator = type.blocks.begin();
			for (auto& block : type.blocks) {
				if (block.maxAllocatableSize == block.originalSize) {
					if (block.originalSize > m_blockSize) {
						vkFreeMemory(m_context->device(), block.memoryHandle, nullptr);
						type.blocks.removeElement(type.blocks.handle(iterator));
						// no way to handle iterator invalidation gracefully, restart loop
						goto blockFreeStart;
					} else if (++freeBlockCount > 1) {
						vkFreeMemory(m_context->device(), block.memoryHandle, nullptr);
						type.blocks.removeElement(type.blocks.handle(iterator));
						goto blockFreeStart;
					}
				}
				++iterator;
			}
		imageBlockFreeStart:
			auto imageIterator = type.imageBlocks.begin();
			for (auto& block : type.imageBlocks) {
				if (block.maxAllocatableSize == block.originalSize) {
					if (block.originalSize > m_blockSize) {
						vkFreeMemory(m_context->device(), block.memoryHandle, nullptr);
						type.imageBlocks.removeElement(type.imageBlocks.handle(imageIterator));
						// no way to handle iterator invalidation gracefully, restart loop
						goto imageBlockFreeStart;
					} else if (++freeBlockCount > 1) {
						vkFreeMemory(m_context->device(), block.memoryHandle, nullptr);
						type.imageBlocks.removeElement(type.imageBlocks.handle(imageIterator));
						goto imageBlockFreeStart;
					}
				}
				++imageIterator;
			}
		}

		for (auto& block : m_blockFreeList[m_currentFrameIndex]) {
			vkFreeMemory(m_context->device(), block.memoryHandle, nullptr);
		}
		m_blockFreeList[m_currentFrameIndex].clear();
	}

	uint32_t GPUResourceAllocator::bestTypeIndex(VkMemoryPropertyFlags required, VkMemoryPropertyFlags preferred,
												 VkMemoryRequirements requirements, bool createMapped) {
		uint32_t bestMatchingTypeIndex = ~0U;
		size_t bestNumMatchingCapabilities = 0;
		size_t bestNumUnrelatedCapabilities = ~0U;

		uint32_t typeIndex = 0;
		for (auto& type : m_memoryTypes) {
			if ((type.properties & required) != required) {
				++typeIndex;
				continue;
			}

			size_t numMatchingCapabilities = std::popcount(preferred & type.properties);
			size_t numUnrelatedCapabilities = std::popcount((~preferred) & type.properties);
			if (numMatchingCapabilities > bestNumMatchingCapabilities ||
				(numMatchingCapabilities == bestNumMatchingCapabilities &&
				 numUnrelatedCapabilities < bestNumUnrelatedCapabilities) &&
					((1U << typeIndex) & requirements.memoryTypeBits)) {
				if (!isTypeBigEnough(typeIndex, requirements.size, createMapped)) {
					++typeIndex;
					continue;
				}
				bestMatchingTypeIndex = typeIndex;
				bestNumMatchingCapabilities = numMatchingCapabilities;
				bestNumUnrelatedCapabilities = numUnrelatedCapabilities;
			}
			++typeIndex;
		}

		return bestMatchingTypeIndex;
	}

	bool GPUResourceAllocator::isTypeBigEnough(uint32_t typeIndex, VkDeviceSize size, bool createMapped) {
		for (auto& block : m_memoryTypes[typeIndex].blocks) {
			if (block.maxAllocatableSize >= size && (!createMapped || block.mappedPointer != nullptr)) {
				return true;
			}
		}
		if (m_heapBudgets[m_memoryTypes[typeIndex].heapIndex] > size)
			return true;
		return false;
	}

	std::optional<AllocationResult> GPUResourceAllocator::allocate(uint32_t typeIndex, VkDeviceSize alignment,
																   VkDeviceSize size, bool createMapped) {
		auto blockIterator = m_memoryTypes[typeIndex].blocks.begin();
		for (auto& block : m_memoryTypes[typeIndex].blocks) {
			if (block.maxAllocatableSize >= size &&
				(!createMapped || !block.capabilities.hostVisible || block.mappedPointer != nullptr)) {
				auto result = allocateInBlock(m_memoryTypes[typeIndex].blocks.handle(blockIterator), block,
											  alignment, size, createMapped);
				if (result.has_value())
					return result;
			}
			++blockIterator;
		}
		if (m_heapBudgets[m_memoryTypes[typeIndex].heapIndex] > size) {
			if (!allocateBlock(typeIndex, size, createMapped, false)) {
				return std::nullopt;
			}
			auto blockHandle = m_memoryTypes[typeIndex].blocks.handle(--m_memoryTypes[typeIndex].blocks.end());
			return allocateInBlock(blockHandle, *(--m_memoryTypes[typeIndex].blocks.end()), alignment, size,
								   createMapped);
		}
		return std::nullopt;
	}

	std::optional<AllocationResult> GPUResourceAllocator::allocateImage(uint32_t typeIndex, VkDeviceSize alignment,
																		VkDeviceSize size) {
		auto blockIterator = m_memoryTypes[typeIndex].imageBlocks.begin();
		for (auto& block : m_memoryTypes[typeIndex].imageBlocks) {
			if (block.maxAllocatableSize >= size) {
				auto result = allocateInBlock(m_memoryTypes[typeIndex].imageBlocks.handle(blockIterator),
											  block, alignment, size, false);
				if (result.has_value())
					return result;
			}
			++blockIterator;
		}
		if (m_heapBudgets[m_memoryTypes[typeIndex].heapIndex] > size) {
			if (!allocateBlock(typeIndex, size, false, true)) {
				return std::nullopt;
			}
			auto blockHandle =
				m_memoryTypes[typeIndex].imageBlocks.handle(--m_memoryTypes[typeIndex].imageBlocks.end());
			return allocateInBlock(blockHandle, *(--m_memoryTypes[typeIndex].imageBlocks.end()), alignment,
								   size, false);
		}
		return std::nullopt;
	}

	std::optional<AllocationResult> GPUResourceAllocator::allocateInBlock(BlockHandle blockHandle,
																		  MemoryBlock& block, VkDeviceSize alignment,
																		  VkDeviceSize size, bool createMapped) {
		auto result = allocateFromRanges(block.freeBlocksOffsetSorted, block.freeBlocksSizeSorted, alignment, size);
		if (block.freeBlocksSizeSorted.empty())
			block.maxAllocatableSize = 0U;
		else {
			block.maxAllocatableSize = block.freeBlocksSizeSorted.back().size;
		}

		if (result.has_value()) {
			return AllocationResult{ .allocationRange = result.value().allocationRange,
									 .usableRange = result.value().usableRange,
									 .blockHandle = blockHandle };
		} else {
			return std::nullopt;
		}
	}

	void GPUResourceAllocator::freeInBlock(MemoryBlock& block, VkDeviceSize offset, VkDeviceSize size) {
		freeToRanges(block.freeBlocksOffsetSorted, block.freeBlocksSizeSorted, offset, size);
		block.maxAllocatableSize = block.freeBlocksSizeSorted.back().size;
	}

	bool GPUResourceAllocator::allocateBlock(uint32_t typeIndex, VkDeviceSize size, bool createMapped,
											 bool createImageBlock) {
		size = std::max(m_blockSize, size);
		VkDeviceMemory newMemory;
		VkMemoryAllocateInfo info = { .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
									  .allocationSize = size,
									  .memoryTypeIndex = typeIndex };

		VkResult result = vkAllocateMemory(m_context->device(), &info, nullptr, &newMemory);

		if (result == VK_ERROR_OUT_OF_DEVICE_MEMORY) {
			m_heapBudgets[m_memoryTypes[typeIndex].heapIndex] = size - 1;
			return false;
		}
		verifyResult(result);

		MemoryCapabilities capabilities = {
			.deviceLocal = static_cast<bool>(m_memoryTypes[typeIndex].properties & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
			.hostVisible = static_cast<bool>(m_memoryTypes[typeIndex].properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT),
			.hostCoherent =
				static_cast<bool>(m_memoryTypes[typeIndex].properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
		};

		void* mappedPointer = nullptr;
		if (createMapped) {
			verifyResult(vkMapMemory(m_context->device(), newMemory, 0, size, 0, &mappedPointer));
		}

		MemoryBlock block = { .freeBlocksSizeSorted = { { .offset = 0, .size = size } },
							  .freeBlocksOffsetSorted = { { .offset = 0, .size = size } },
							  .capabilities = capabilities,
							  .maxAllocatableSize = size,
							  .originalSize = size,
							  .memoryHandle = newMemory,
							  .mappedPointer = mappedPointer };
		if (createImageBlock)
			m_memoryTypes[typeIndex].imageBlocks.addElement(block);
		else
			m_memoryTypes[typeIndex].blocks.addElement(block);

		if (m_heapBudgets[m_memoryTypes[typeIndex].heapIndex] < size)
			m_heapBudgets[m_memoryTypes[typeIndex].heapIndex] = size;

		m_heapBudgets[m_memoryTypes[typeIndex].heapIndex] -= size;

		return true;
	}

	bool GPUResourceAllocator::allocateCustomBlock(uint32_t typeIndex, VkDeviceSize size, bool createMapped,
												   bool createImageBlock) {
		VkDeviceMemory newMemory;
		VkMemoryAllocateInfo info = { .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
									  .allocationSize = size,
									  .memoryTypeIndex = typeIndex };

		VkResult result = vkAllocateMemory(m_context->device(), &info, nullptr, &newMemory);

		if (result == VK_ERROR_OUT_OF_DEVICE_MEMORY) {
			m_heapBudgets[m_memoryTypes[typeIndex].heapIndex] = size - 1;
			return false;
		}
		verifyResult(result);

		MemoryCapabilities capabilities = {
			.deviceLocal = static_cast<bool>(m_memoryTypes[typeIndex].properties & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
			.hostVisible = static_cast<bool>(m_memoryTypes[typeIndex].properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT),
			.hostCoherent =
				static_cast<bool>(m_memoryTypes[typeIndex].properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
		};

		void* mappedPointer = nullptr;
		if (createMapped) {
			verifyResult(vkMapMemory(m_context->device(), newMemory, 0, size, 0, &mappedPointer));
		}

		MemoryBlock block = { .freeBlocksSizeSorted = { { .offset = 0, .size = size } },
							  .freeBlocksOffsetSorted = { { .offset = 0, .size = size } },
							  .capabilities = capabilities,
							  .maxAllocatableSize = size,
							  .originalSize = size,
							  .memoryHandle = newMemory,
							  .mappedPointer = mappedPointer };
		if (createImageBlock)
			m_customImageBlocks.addElement(block);
		else
			m_customBufferBlocks.addElement(block);

		if (m_heapBudgets[m_memoryTypes[typeIndex].heapIndex] < size)
			m_heapBudgets[m_memoryTypes[typeIndex].heapIndex] = size;

		m_heapBudgets[m_memoryTypes[typeIndex].heapIndex] -= size;

		return true;
	}
} // namespace vanadium::graphics