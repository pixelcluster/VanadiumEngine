#include <graphics/assets/AssetStreamer.hpp>

namespace vanadium::graphics {
	void AssetStreamer::create(AssetLibrary* library, GPUResourceAllocator* resourceAllocator,
							   GPUTransferManager* transferManager) {
		m_library = library;
		m_resourceAllocator = resourceAllocator;
		m_transferManager = transferManager;

		m_bufferStreamPool = resourceAllocator->createBufferBlock(m_bufferPoolSize, {}, {
			.deviceLocal = true
		}, false);
		m_imageStreamPool = resourceAllocator->createImageBlock(m_imagePoolSize, {}, {
			.deviceLocal = true
		});
	}

	bool AssetStreamer::declareMeshUsage(uint32_t id, bool createTransfer) {
		auto lock = std::lock_guard<std::shared_mutex>(m_accessMutex);
		if(m_bufferResourceStates[id].isLoaded) {
			return true;
		}
		else {
			if(createTransfer && !m_bufferResourceStates[id].isLoading) {
				
			}
		}
	}
} // namespace vanadium::graphics
