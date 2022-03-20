#include <graphics/assets/AssetStreamer.hpp>

namespace vanadium::graphics {
	void AssetStreamer::create(AssetLibrary* library, GPUResourceAllocator* resourceAllocator,
							   GPUTransferManager* transferManager) {
		m_library = library;
		m_resourceAllocator = resourceAllocator;
		m_transferManager = transferManager;
	}
} // namespace vanadium::graphics
