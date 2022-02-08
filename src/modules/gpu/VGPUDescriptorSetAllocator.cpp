#include <modules/gpu/VGPUDescriptorSetAllocator.hpp>
#include <modules/gpu/helper/ErrorHelper.hpp>
#include <volk.h>

struct VDescriptorAllocationBatch {
	VDescriptorSizeClassHandle sizeClassHandle;
	VDescriptorPoolHandle poolHandle;

	std::vector<size_t> allocationIndices;
};

VGPUDescriptorSetAllocator::VGPUDescriptorSetAllocator() {}

void VGPUDescriptorSetAllocator::create(VGPUContext* context) {
	m_context = context;
	m_poolFreeLists.resize(frameInFlightCount);
}

std::vector<VDescriptorSetAllocation> VGPUDescriptorSetAllocator::allocateDescriptorSets(
	const std::vector<VDescriptorSetAllocationInfo>& infos) {
	auto allocations = std::vector<VDescriptorSetAllocation>(infos.size());

	std::vector<VDescriptorAllocationBatch> allocatedBatches;

	size_t allocationIndex = 0;
	for (auto& info : infos) {
		bool allocationSucceeded = false;
		for (auto& batch : allocatedBatches) {
			if (isSizeClassCompatible(m_sizeClasses[batch.sizeClassHandle], info) &&
				tryAllocate(m_sizeClasses[batch.sizeClassHandle], info, batch.poolHandle)) {
				allocationSucceeded = true;
				batch.allocationIndices.push_back(allocationIndex);
				break;
			}
		}

		if (allocationSucceeded) {
			++allocationIndex;
			continue;
		}

		for (auto iterator = m_sizeClasses.begin(); iterator != m_sizeClasses.end(); ++iterator) {
			if (isSizeClassCompatible(*iterator, info)) {
				allocationSucceeded = false;
				for (auto poolIterator = iterator->pools.begin(); poolIterator != iterator->pools.end();
					 ++poolIterator) {
					if (tryAllocate(*iterator, info, iterator->pools.handle(poolIterator))) {
						allocatedBatches.push_back({ .sizeClassHandle = m_sizeClasses.handle(iterator),
													 .poolHandle = iterator->pools.handle(poolIterator),
													 .allocationIndices = { allocationIndex } });
						allocationSucceeded = true;
						break;
					}
				}
				if (!allocationSucceeded) {
					addPool(*iterator);
					tryAllocate(*iterator, info, iterator->pools.handle(--iterator->pools.end()));
					allocatedBatches.push_back({ .sizeClassHandle = m_sizeClasses.handle(iterator),
												 .poolHandle = iterator->pools.handle(--iterator->pools.end()),
												 .allocationIndices = { allocationIndex } });
					allocationSucceeded = true;
				}
				break;
			}
		}
		if (!allocationSucceeded) {
			addSizeClass(info);
			auto iterator = --m_sizeClasses.end();
			tryAllocate(*iterator, info, iterator->pools.handle(--iterator->pools.end()));
			allocatedBatches.push_back({ .sizeClassHandle = m_sizeClasses.handle(iterator),
										 .poolHandle = iterator->pools.handle(--iterator->pools.end()),
										 .allocationIndices = { allocationIndex } });
		}

		++allocationIndex;
	}

	for (auto& batch : allocatedBatches) {
		auto dstDescriptorSets = std::vector<VkDescriptorSet>(batch.allocationIndices.size());

		std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
		descriptorSetLayouts.reserve(batch.allocationIndices.size());
		for (auto index : batch.allocationIndices) {
			descriptorSetLayouts.push_back(infos[index].layout);
		}

		VkDescriptorSetAllocateInfo allocateInfo = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool = m_sizeClasses[batch.sizeClassHandle].pools[batch.poolHandle].pool,
			.descriptorSetCount = static_cast<uint32_t>(descriptorSetLayouts.size()),
			.pSetLayouts = descriptorSetLayouts.data()
		};
		verifyResult(vkAllocateDescriptorSets(m_context->device(), &allocateInfo, dstDescriptorSets.data()));

		size_t batchIndex = 0;
		for (auto index : batch.allocationIndices) {
			allocations[index].set = dstDescriptorSets[batchIndex];
			allocations[index].poolHandle = batch.poolHandle;
			allocations[index].sizeClassHandle = batch.sizeClassHandle;

			++batchIndex;
		}
	}
	return allocations;
}

void VGPUDescriptorSetAllocator::freeDescriptorSet(const VDescriptorSetAllocation& allocation,
												   const VDescriptorSetAllocationInfo& info) {
	auto& pool = m_sizeClasses[allocation.sizeClassHandle].pools[allocation.poolHandle];
	if (++pool.m_freedSets == m_setsPerPool) {
		m_poolFreeLists[m_currentFrameIndex].push_back(pool.pool);
		m_sizeClasses[allocation.sizeClassHandle].pools.removeElement(allocation.poolHandle);
	} else {
		for (auto& type : info.typeInfos) {
			auto countIterator = pool.numRemainingDescriptors.find(type.type);
			if (countIterator != pool.numRemainingDescriptors.end()) {
				countIterator->second += type.count;
			}
		}
	}
}

void VGPUDescriptorSetAllocator::setCurrentFrameIndex(uint32_t newFrameIndex) {
	m_currentFrameIndex = newFrameIndex;
	flushFreeList();
}

void VGPUDescriptorSetAllocator::destroy() {
	for (auto& sizeClass : m_sizeClasses) {
		for (auto& pool : sizeClass.pools) {
			m_poolFreeLists[m_currentFrameIndex].push_back(pool.pool);
		}
	}
	m_sizeClasses.clear();

	for (uint32_t i = 0; i < frameInFlightCount; ++i) {
		// flush all free lists
		setCurrentFrameIndex(i);
	}
}

bool VGPUDescriptorSetAllocator::isSizeClassCompatible(const VDescriptorSetSizeClass& sizeClass,
													   const VDescriptorSetAllocationInfo& info) {
	if (info.typeInfos.empty())
		return false;

	float sizeClassDeviation = 0.0f;
	for (auto& typeInfo : info.typeInfos) {
		float typeDescriptorCount = 0.0f;
		auto typeIterator = sizeClass.descriptorsPerSet.find(typeInfo.type);
		if (typeIterator != sizeClass.descriptorsPerSet.end()) {
			typeDescriptorCount = typeIterator->second;
		}
		float quotient = static_cast<float>(typeInfo.count) / typeDescriptorCount;
		sizeClassDeviation += quotient * quotient;
	}

	sizeClassDeviation /= info.typeInfos.size();

	return sizeClassDeviation < 1.2f;
}

bool VGPUDescriptorSetAllocator::tryAllocate(VDescriptorSetSizeClass& sizeClass,
											 const VDescriptorSetAllocationInfo& info, VDescriptorPoolHandle pool) {
	auto& poolInfo = sizeClass.pools[pool];
	if (poolInfo.m_remainingSets) {
		bool fitsDescriptorTypes = true;
		for (auto& info : info.typeInfos) {
			uint32_t descriptorCount = 0;
			auto countIterator = poolInfo.numRemainingDescriptors.find(info.type);
			if (countIterator != poolInfo.numRemainingDescriptors.end()) {
				descriptorCount = countIterator->second;
			}
			fitsDescriptorTypes &= info.count >= descriptorCount;
		}

		if (fitsDescriptorTypes) {
			--poolInfo.m_remainingSets;
			for (auto& info : info.typeInfos) {
				auto countIterator = poolInfo.numRemainingDescriptors.find(info.type);
				if (countIterator != poolInfo.numRemainingDescriptors.end()) {
					countIterator->second -= info.count;
				}
			}
			return true;
		}
	}
	return false;
}

void VGPUDescriptorSetAllocator::addSizeClass(const VDescriptorSetAllocationInfo& info) {
	VDescriptorSetSizeClass sizeClass = {};
	sizeClass.descriptorsPerSet.reserve(info.typeInfos.size());

	for (auto& info : info.typeInfos) {
		sizeClass.descriptorsPerSet.insert({ info.type, static_cast<uint32_t>(info.count) });
	}
	addPool(sizeClass);

	m_sizeClasses.addElement(std::move(sizeClass));
}

void VGPUDescriptorSetAllocator::addPool(VDescriptorSetSizeClass& sizeClass) {
	std::vector<VkDescriptorPoolSize> poolSizes;
	poolSizes.reserve(sizeClass.descriptorsPerSet.size());

	VDescriptorPoolInfo info = { .m_remainingSets = m_setsPerPool };
	info.numRemainingDescriptors.reserve(sizeClass.descriptorsPerSet.size());

	for (auto& descriptorCount : sizeClass.descriptorsPerSet) {
		poolSizes.push_back(
			{ .type = descriptorCount.first, .descriptorCount = descriptorCount.second * m_setsPerPool });
		info.numRemainingDescriptors.insert({ descriptorCount.first, descriptorCount.second * m_setsPerPool });
	}

	VkDescriptorPoolCreateInfo poolCreateInfo = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
												  .maxSets = m_setsPerPool,
												  .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
												  .pPoolSizes = poolSizes.data() };
	verifyResult(vkCreateDescriptorPool(m_context->device(), &poolCreateInfo, nullptr, &info.pool));

	sizeClass.pools.addElement(std::move(info));
}

void VGPUDescriptorSetAllocator::flushFreeList() {
	for (auto& pool : m_poolFreeLists[m_currentFrameIndex]) {
		vkDestroyDescriptorPool(m_context->device(), pool, nullptr);
	}
	m_poolFreeLists[m_currentFrameIndex].clear();
}
