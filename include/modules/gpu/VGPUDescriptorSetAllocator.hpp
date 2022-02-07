#pragma once

#include <modules/gpu/VGPUContext.hpp>
#include <helper/VSlotmap.hpp>

struct VDescriptorPoolInfo {
	std::unordered_map<VkDescriptorType, size_t> numRemainingDescriptors;
	VkDescriptorPool pool;

	size_t m_remainingSets;
	size_t m_freedSets;
};

struct VDescriptorSetSizeClass {
	VSlotmap<VDescriptorPoolInfo> pools;
	std::unordered_map<VkDescriptorType, uint32_t> descriptorsPerSet;
};

struct VDescriptorTypeInfo {
	VkDescriptorType type;
	size_t count;
};

using VDescriptorPoolHandle = VSlotmapHandle;
using VDescriptorSizeClassHandle = VSlotmapHandle;

struct VDescriptorSetAllocationInfo {
	std::vector<VDescriptorTypeInfo> typeInfos;
	VkDescriptorSetLayout layout;
};

struct VDescriptorSetAllocation {
	VDescriptorPoolHandle poolHandle;
	VDescriptorSizeClassHandle sizeClassHandle;
	VkDescriptorSet set;
};

class VGPUDescriptorSetAllocator {
  public:
	VGPUDescriptorSetAllocator(VGPUContext* context);

	std::vector<VDescriptorSetAllocation> allocateDescriptorSets(const std::vector<VDescriptorSetAllocationInfo>& infos);
	void freeDescriptorSet(const VDescriptorSetAllocation& allocation, const VDescriptorSetAllocationInfo& info);

	void setCurrentFrameIndex(uint32_t newFrameIndex);

	void destroy();
  private:
	static constexpr uint32_t m_setsPerPool = 128;

	bool isSizeClassCompatible(const VDescriptorSetSizeClass& sizeClass, const VDescriptorSetAllocationInfo& info);
	bool tryAllocate(VDescriptorSetSizeClass& sizeClass,
													 const VDescriptorSetAllocationInfo& info, VDescriptorPoolHandle pool);
	void addSizeClass(const VDescriptorSetAllocationInfo& info);
	void addPool(VDescriptorSetSizeClass& sizeClass);

	void flushFreeList();

	VGPUContext* m_context;
	uint32_t m_currentFrameIndex = 0;

	VSlotmap<VDescriptorSetSizeClass> m_sizeClasses;

	std::vector<std::vector<VkDescriptorPool>> m_poolFreeLists;
};