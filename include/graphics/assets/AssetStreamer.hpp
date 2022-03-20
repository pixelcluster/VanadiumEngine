#pragma once
#include <graphics/assets/AssetLibrary.hpp>
#include <graphics/util/GPUResourceAllocator.hpp>
#include <graphics/util/GPUTransferManager.hpp>
#include <robin_hood.h>

namespace vanadium::graphics {
	class AssetStreamer {
	  public:
		void create(AssetLibrary* library, GPUResourceAllocator* resourceAllocator,
					GPUTransferManager* transferManager);

		bool declareImageUsage(uint32_t id, bool createTransfer);
		bool declareMeshUsage(uint32_t id, bool createTransfer);

	  private:
		AssetLibrary* m_library;
		GPUResourceAllocator* m_resourceAllocator;
		GPUTransferManager* m_transferManager;
	};

} // namespace vanadium::graphics
