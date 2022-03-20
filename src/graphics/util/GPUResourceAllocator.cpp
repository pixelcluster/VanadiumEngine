#include <graphics/helper/ErrorHelper.hpp>
#include <graphics/util/GPUResourceAllocator.hpp>
#include <volk.h>

#include <bit>

namespace vanadium::graphics {

	void GPUResourceAllocator::create(DeviceContext* gpuContext) {
		m_bufferFreeList.resize(frameInFlightCount);
		m_imageFreeList.resize(frameInFlightCount);

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
		VkMemoryPropertyFlags requiredFlags =
			(required.deviceLocal ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT : 0) |
			(required.hostCoherent ? VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : 0) |
			((required.hostVisible || createMapped) ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT : 0);
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
		// clang-format off
	VkMemoryPropertyFlags requiredFlags = (required.deviceLocal  ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT  : 0) | 
										  (required.hostCoherent ? VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : 0) |
						((required.hostVisible || createMapped)  ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT  : 0);
	VkMemoryPropertyFlags preferredFlags = (preferred.deviceLocal  ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT  : 0) | 
										   (preferred.hostCoherent ? VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : 0) |
										   (preferred.hostVisible  ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT  : 0);
		// clang-format on

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
		}
	}

	MemoryCapabilities GPUResourceAllocator::bufferMemoryCapabilities(BufferResourceHandle handle) {
		return m_memoryTypes[m_buffers[handle].typeIndex].blocks[m_buffers[handle].blockHandle].capabilities;
	}

	VkDeviceMemory GPUResourceAllocator::nativeMemoryHandle(BufferResourceHandle handle) {
		return m_memoryTypes[m_buffers[handle].typeIndex].blocks[m_buffers[handle].blockHandle].memoryHandle;
	}

	MemoryRange GPUResourceAllocator::allocationRange(BufferResourceHandle handle) {
		return m_buffers[handle].bufferContentRange;
	}

	void* GPUResourceAllocator::mappedBufferData(BufferResourceHandle handle) {
		return m_buffers[handle].mappedData[m_currentFrameIndex];
	}

	void GPUResourceAllocator::destroyBuffer(BufferResourceHandle handle) {
		auto& allocation = m_buffers[handle];
		m_bufferFreeList[m_currentFrameIndex].push_back(allocation);
		m_buffers.removeElement(handle);
	}

	ImageResourceHandle GPUResourceAllocator::createImage(const VkImageCreateInfo& imageCreateInfo,
														  MemoryCapabilities required, MemoryCapabilities preferred) {
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

	VkImage GPUResourceAllocator::nativeImageHandle(ImageResourceHandle handle) { return m_images[handle].image; }

	const ImageResourceInfo& GPUResourceAllocator::imageResourceInfo(ImageResourceHandle handle) {
		return m_images[handle].resourceInfo;
	}

	VkImageView GPUResourceAllocator::requestImageView(ImageResourceHandle handle, const ImageResourceViewInfo& info) {
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
		auto& allocation = m_images[handle];
		m_imageFreeList[m_currentFrameIndex].push_back(allocation);
		m_images.removeElement(handle);
	}

	void GPUResourceAllocator::destroyBufferBlock(BlockHandle handle) {
		m_blockFreeList[m_currentFrameIndex].push_back(m_customBufferBlocks[handle]);
		m_customBufferBlocks.removeElement(handle);
	}
	void GPUResourceAllocator::destroyImageBlock(BlockHandle handle) {
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
		m_currentFrameIndex = frameIndex;
		flushFreeList();
	}

	void GPUResourceAllocator::updateMemoryBudget() {
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
		m_bufferFreeList[m_currentFrameIndex].clear();

		for (auto& allocation : m_imageFreeList[m_currentFrameIndex]) {
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
		m_imageFreeList[m_currentFrameIndex].clear();

		for (auto& type : m_memoryTypes) {
			size_t freeBlockCount = 0;

		blockFreeStart:
			for (auto& block : type.blocks) {
				if (block.maxAllocatableSize == block.originalSize) {
					if (block.originalSize > m_blockSize) {
						vkFreeMemory(m_context->device(), block.memoryHandle, nullptr);
						// no way to handle iterator invalidation gracefully, restart loop
						goto blockFreeStart;
					} else if (++freeBlockCount > 1) {
						vkFreeMemory(m_context->device(), block.memoryHandle, nullptr);
						goto blockFreeStart;
					}
				}
			}
		imageBlockFreeStart:
			for (auto& block : type.imageBlocks) {
				if (block.maxAllocatableSize == block.originalSize) {
					if (block.originalSize > m_blockSize) {
						vkFreeMemory(m_context->device(), block.memoryHandle, nullptr);
						// no way to handle iterator invalidation gracefully, restart loop
						goto imageBlockFreeStart;
					} else if (++freeBlockCount > 1) {
						vkFreeMemory(m_context->device(), block.memoryHandle, nullptr);
						goto imageBlockFreeStart;
					}
				}
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
			if (block.maxAllocatableSize >= size && (!createMapped || block.mappedPointer != nullptr)) {
				auto result = allocateInBlock(typeIndex, m_memoryTypes[typeIndex].blocks.handle(blockIterator), block,
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
			return allocateInBlock(typeIndex, blockHandle, *(--m_memoryTypes[typeIndex].blocks.end()), alignment, size,
								   createMapped);
		}
		return std::nullopt;
	}

	std::optional<AllocationResult> GPUResourceAllocator::allocateImage(uint32_t typeIndex, VkDeviceSize alignment,
																		VkDeviceSize size) {
		auto blockIterator = m_memoryTypes[typeIndex].imageBlocks.begin();
		for (auto& block : m_memoryTypes[typeIndex].imageBlocks) {
			if (block.maxAllocatableSize >= size) {
				auto result = allocateInBlock(typeIndex, m_memoryTypes[typeIndex].imageBlocks.handle(blockIterator),
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
			return allocateInBlock(typeIndex, blockHandle, *(--m_memoryTypes[typeIndex].imageBlocks.end()), alignment,
								   size, false);
		}
		return std::nullopt;
	}

	std::optional<AllocationResult> GPUResourceAllocator::allocateInBlock(uint32_t typeIndex, BlockHandle blockHandle,
																		  MemoryBlock& block, VkDeviceSize alignment,
																		  VkDeviceSize size, bool createMapped) {
		auto offsetComparator = [](const MemoryRange& one, const MemoryRange& other) {
			return one.offset < other.offset;
		};
		auto sizeComparator = [](const MemoryRange& one, const MemoryRange& other) { return one.size < other.size; };

		// seek for smallest available block
		size_t allocationIndex = block.freeBlocksSizeSorted.size() - 1;
		while (allocationIndex < block.freeBlocksSizeSorted.size() &&
			   block.freeBlocksSizeSorted[allocationIndex].size >=
				   size + alignmentMargin(block.freeBlocksSizeSorted[allocationIndex].offset, alignment)) {
			--allocationIndex;
		}
		++allocationIndex;
		if (allocationIndex == block.freeBlocksSizeSorted.size()) {
			return std::nullopt;
		}

		VkDeviceSize margin = alignmentMargin(block.freeBlocksSizeSorted[allocationIndex].offset, alignment);
		auto& usedRange = block.freeBlocksSizeSorted[allocationIndex];

		AllocationResult result = { .allocationRange = { .offset = usedRange.offset, .size = size + margin },
									.usableRange = { .offset = usedRange.offset + margin, .size = size },
									.blockHandle = blockHandle };

		auto& range = block.freeBlocksSizeSorted[allocationIndex];
		auto offsetIterator = std::lower_bound(block.freeBlocksOffsetSorted.begin(), block.freeBlocksOffsetSorted.end(),
											   range, offsetComparator);

		if (result.allocationRange.size != range.size) {
			// make unused part of block another free range
			range.offset += result.allocationRange.size;
			range.size -= result.allocationRange.size;
			offsetIterator->offset += result.allocationRange.size;
			offsetIterator->size -= result.allocationRange.size;
			reorderOffsetArea(block, allocationIndex);
			reorderSizeArea(block, allocationIndex);
		} else {
			block.freeBlocksOffsetSorted.erase(offsetIterator);
			auto sizeIterator = std::lower_bound(block.freeBlocksSizeSorted.begin(), block.freeBlocksSizeSorted.end(),
												 range, sizeComparator);
			block.freeBlocksSizeSorted.erase(sizeIterator);
		}
		if (block.freeBlocksSizeSorted.empty())
			block.maxAllocatableSize = 0U;
		else {
			block.maxAllocatableSize = block.freeBlocksSizeSorted.back().size;
		}

		return result;
	}

	void GPUResourceAllocator::freeInBlock(MemoryBlock& block, VkDeviceSize offset, VkDeviceSize size) {

		auto offsetComparator = [](const MemoryRange& one, const MemoryRange& other) {
			return one.offset < other.offset;
		};
		auto sizeComparator = [](const MemoryRange& one, const MemoryRange& other) { return one.size < other.size; };

		if (block.freeBlocksOffsetSorted.empty()) {
			MemoryRange range = { .offset = offset, .size = size };
			block.freeBlocksOffsetSorted.push_back(range);
			block.freeBlocksSizeSorted.push_back(range);
			block.maxAllocatableSize = block.freeBlocksSizeSorted.back().size;
			return;
		}

		MemoryRange range = { .offset = offset, .size = size };

		auto offsetInsertIterator = std::lower_bound(block.freeBlocksOffsetSorted.begin(),
													 block.freeBlocksOffsetSorted.end(), range, offsetComparator);

		if (offsetInsertIterator == block.freeBlocksOffsetSorted.end()) {
			block.freeBlocksOffsetSorted.push_back(range);
		} else {
			block.freeBlocksOffsetSorted.insert(offsetInsertIterator, range);
		}

		auto sizeInsertIterator = std::lower_bound(block.freeBlocksSizeSorted.begin(), block.freeBlocksSizeSorted.end(),
												   range, sizeComparator);

		if (sizeInsertIterator == block.freeBlocksSizeSorted.end())
			block.freeBlocksSizeSorted.push_back(range);
		else
			block.freeBlocksSizeSorted.insert(sizeInsertIterator, range);

		mergeFreeAreas(block);

		block.maxAllocatableSize = block.freeBlocksSizeSorted.back().size;

		return;
	}

	void GPUResourceAllocator::mergeFreeAreas(MemoryBlock& block) {
		auto sizeComparator = [](const MemoryRange& one, const MemoryRange& other) { return one.size < other.size; };

		if (block.freeBlocksOffsetSorted.empty())
			return;
		for (size_t i = 0; i < block.freeBlocksOffsetSorted.size() - 1; ++i) {
			auto& area = block.freeBlocksOffsetSorted[i];
			auto& nextArea = block.freeBlocksOffsetSorted[i + 1];

			if (area.offset + area.size == nextArea.offset) {
				area.size += nextArea.size;
				block.freeBlocksOffsetSorted.erase(block.freeBlocksOffsetSorted.begin() + i + 1);
				reorderSizeArea(block, i);
				--i;
			}
		}
	}

	void GPUResourceAllocator::reorderOffsetArea(MemoryBlock& block, size_t index) {
		auto offsetComparator = [](const MemoryRange& one, const MemoryRange& other) {
			return one.offset < other.offset;
		};
		auto range = block.freeBlocksOffsetSorted[index];
		auto reinsertIterator = std::lower_bound(block.freeBlocksOffsetSorted.begin(),
												 block.freeBlocksOffsetSorted.end(), range, offsetComparator);

		if (reinsertIterator == block.freeBlocksOffsetSorted.end() ||
			reinsertIterator == block.freeBlocksOffsetSorted.end() - 1) {
			block.freeBlocksOffsetSorted.erase(block.freeBlocksOffsetSorted.begin() + index);
			block.freeBlocksOffsetSorted.push_back(range);
		} else {
			for (auto iterator = block.freeBlocksOffsetSorted.begin() + index; iterator != reinsertIterator;
				 ++iterator) {
				*iterator = std::move(*(iterator + 1));
			}
			*reinsertIterator = range;
		}
	}

	void GPUResourceAllocator::reorderSizeArea(MemoryBlock& block, size_t index) {
		auto sizeComparator = [](const MemoryRange& one, const MemoryRange& other) { return one.size < other.size; };

		auto range = block.freeBlocksSizeSorted[index];
		auto reinsertIterator = std::lower_bound(block.freeBlocksSizeSorted.begin(), block.freeBlocksSizeSorted.end(),
												 range, sizeComparator);

		if (reinsertIterator == block.freeBlocksSizeSorted.end() ||
			reinsertIterator == block.freeBlocksSizeSorted.end() - 1) {
			block.freeBlocksSizeSorted.erase(block.freeBlocksSizeSorted.begin() + index);
			block.freeBlocksSizeSorted.push_back(range);
		} else {
			for (auto iterator = block.freeBlocksSizeSorted.begin() + index; iterator != reinsertIterator; ++iterator) {
				*iterator = std::move(*(iterator + 1));
			}
			*reinsertIterator = range;
		}
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

	VkDeviceSize GPUResourceAllocator::roundUpAligned(VkDeviceSize n, VkDeviceSize alignment) {
		return n + alignmentMargin(n, alignment);
	}

	VkDeviceSize GPUResourceAllocator::alignmentMargin(VkDeviceSize n, VkDeviceSize alignment) {
		if (alignment) {
			VkDeviceSize remainder = n % alignment;
			if (remainder) {
				return alignment - remainder;
			}
		}
		return 0;
	}
} // namespace vanadium::graphics