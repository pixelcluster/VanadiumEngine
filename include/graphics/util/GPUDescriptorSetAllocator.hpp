#pragma once

#include <graphics/DeviceContext.hpp>
#include <helper/Slotmap.hpp>
#include <shared_mutex>

namespace vanadium::graphics {

	struct DescriptorPoolInfo {
		robin_hood::unordered_map<VkDescriptorType, size_t> numRemainingDescriptors;
		VkDescriptorPool pool;

		size_t m_remainingSets;
		size_t m_freedSets;
	};

	struct DescriptorSetSizeClass {
		Slotmap<DescriptorPoolInfo> pools;
		robin_hood::unordered_map<VkDescriptorType, uint32_t> descriptorsPerSet;
	};

	struct DescriptorTypeInfo {
		VkDescriptorType type;
		size_t count;
	};

	using DescriptorPoolHandle = SlotmapHandle;
	using DescriptorSizeClassHandle = SlotmapHandle;

	struct DescriptorSetAllocationInfo {
		std::vector<DescriptorTypeInfo> typeInfos;
		VkDescriptorSetLayout layout;
	};

	struct DescriptorSetAllocation {
		DescriptorPoolHandle poolHandle;
		DescriptorSizeClassHandle sizeClassHandle;
		VkDescriptorSet set;
	};

	class GPUDescriptorSetAllocator {
	  public:
		GPUDescriptorSetAllocator();

		void create(DeviceContext* context);

		std::vector<DescriptorSetAllocation> allocateDescriptorSets(
			const std::vector<DescriptorSetAllocationInfo>& infos);
		void freeDescriptorSet(const DescriptorSetAllocation& allocation, const DescriptorSetAllocationInfo& info);

		void setCurrentFrameIndex(uint32_t newFrameIndex);

		void destroy();

	  private:
		static constexpr uint32_t m_setsPerPool = 128;

		bool isSizeClassCompatible(const DescriptorSetSizeClass& sizeClass, const DescriptorSetAllocationInfo& info);
		bool tryAllocate(DescriptorSetSizeClass& sizeClass, const DescriptorSetAllocationInfo& info,
						 DescriptorPoolHandle pool);
		void addSizeClass(const DescriptorSetAllocationInfo& info);
		void addPool(DescriptorSetSizeClass& sizeClass);

		void flushFreeList();

		DeviceContext* m_context;
		uint32_t m_currentFrameIndex = 0;

		Slotmap<DescriptorSetSizeClass> m_sizeClasses;

		std::vector<std::vector<VkDescriptorPool>> m_poolFreeLists;

		std::shared_mutex m_accessMutex;
	};

} // namespace vanadium::graphics