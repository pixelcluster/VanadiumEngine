/* VanadiumEngine, a Vulkan rendering toolkit
 * Copyright (C) 2022 Friedrich Vock
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#pragma once

#include <Slotmap.hpp>
#include <graphics/DeviceContext.hpp>
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
