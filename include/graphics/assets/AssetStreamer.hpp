#pragma once
#include <graphics/assets/AssetLibrary.hpp>
#include <graphics/util/GPUResourceAllocator.hpp>
#include <graphics/util/GPUTransferManager.hpp>
#include <robin_hood.h>

#include <helper/MemoryLiterals.hpp>
#include <shared_mutex>

namespace vanadium::graphics {

	enum class ResourceResidency {
		Unloaded, Loading, Loaded
	};

	struct BufferResourceState
	{
		ResourceResidency residency;
		BufferResourceHandle loadedHandle;
		AsyncBufferTransferHandle loadingTransferHandle; 
	};
	
	struct ImageResourceState
	{
		ResourceResidency residency;
		ImageResourceHandle loadedHandle;
		AsyncImageTransferHandle loadingTransferHandle; 
	};
	

	class AssetStreamer {
	  public:
		void create(AssetLibrary* library, GPUResourceAllocator* resourceAllocator,
					GPUTransferManager* transferManager);

		bool declareImageUsage(uint32_t id, bool createTransfer);
		bool declareMeshUsage(uint32_t id, bool createTransfer);

	  private:
		constexpr static VkDeviceSize m_bufferPoolSize = 32_MiB;
		constexpr static VkDeviceSize m_imagePoolSize = 256_MiB;

		AssetLibrary* m_library;
		GPUResourceAllocator* m_resourceAllocator;
		GPUTransferManager* m_transferManager;

		robin_hood::unordered_map<uint32_t, BufferResourceState> m_bufferResourceStates;
		robin_hood::unordered_map<uint32_t, ImageResourceState> m_imageResourceStates;

		BlockHandle m_bufferStreamPool;
		BlockHandle m_imageStreamPool;

		std::shared_mutex m_accessMutex;
	};

} // namespace vanadium::graphics
