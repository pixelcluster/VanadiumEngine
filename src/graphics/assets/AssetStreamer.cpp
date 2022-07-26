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
#include <graphics/assets/AssetStreamer.hpp>

namespace vanadium::graphics {
	void AssetStreamer::create(AssetLibrary* library, GPUResourceAllocator* resourceAllocator,
							   GPUTransferManager* transferManager) {
		m_library = library;
		m_resourceAllocator = resourceAllocator;
		m_transferManager = transferManager;

		m_bufferStreamPool = resourceAllocator->createBufferBlock(m_bufferPoolSize, {}, { .deviceLocal = true }, false);
		m_imageStreamPool = resourceAllocator->createImageBlock(m_imagePoolSize, {}, { .deviceLocal = true });
	}

	// TODO: evict unused stuff if new buffer allocation fails

	bool AssetStreamer::declareMeshUsage(uint32_t id, bool createTransfer) {
		auto lock = std::lock_guard<std::shared_mutex>(m_accessMutex);
		switch (m_bufferResourceStates[id].residency) {
			case ResourceResidency::Loaded:
				return true;
			case ResourceResidency::Loading:
				if (m_transferManager->isBufferTransferFinished(m_bufferResourceStates[id].loadingTransferHandle)) {
					m_transferManager->finalizeAsyncBufferTransfer(m_bufferResourceStates[id].loadingTransferHandle);
					m_bufferResourceStates[id].residency = ResourceResidency::Loaded;
					return true;
				} else
					return false;
			case ResourceResidency::Unloaded:
				if (createTransfer) {
					VkBufferCreateInfo createInfo = { .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
													  .size = m_library->mesh(id).dataSize,
													  .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT |
															   VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
															   VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
													  .sharingMode = VK_SHARING_MODE_EXCLUSIVE };
					m_bufferResourceStates[id].loadedHandle =
						m_resourceAllocator->createBuffer(createInfo, m_bufferStreamPool, false);
					m_bufferResourceStates[id].loadingTransferHandle = m_transferManager->createAsyncBufferTransfer(
						m_library->mesh(id).data, m_library->mesh(id).dataSize, m_bufferResourceStates[id].loadedHandle,
						0, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
						VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT);
					m_bufferResourceStates[id].residency = ResourceResidency::Loading;
					return false;
				}
				break;
		}
		return false;
	}

	bool AssetStreamer::declareImageUsage(uint32_t id, bool createTransfer) {
		auto lock = std::lock_guard<std::shared_mutex>(m_accessMutex);
		switch (m_imageResourceStates[id].residency) {
			case ResourceResidency::Loaded:
				return true;
			case ResourceResidency::Loading:
				if (m_transferManager->isImageTransferFinished(m_imageResourceStates[id].loadingTransferHandle)) {
					m_transferManager->finalizeAsyncImageTransfer(m_imageResourceStates[id].loadingTransferHandle);
					m_imageResourceStates[id].residency = ResourceResidency::Loaded;
					return true;
				}
			case ResourceResidency::Unloaded:
				if (createTransfer) {
					auto image = m_library->image(id);
					VkImageCreateInfo createInfo = {
						.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
						.imageType = VK_IMAGE_TYPE_2D,
						.format = image.format,
						.extent = { .width = image.width, .height = image.height, .depth = 1 },
						.mipLevels = image.mipCount,
						.arrayLayers = 1,
						.tiling = VK_IMAGE_TILING_OPTIMAL,
						.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
						.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
						.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
					};
					m_imageResourceStates[id].loadedHandle =
						m_resourceAllocator->createImage(createInfo, m_imageStreamPool);
					m_imageResourceStates[id].loadingTransferHandle = m_transferManager->createAsyncImageTransfer(
						m_library->mesh(id).data, m_library->mesh(id).dataSize, m_imageResourceStates[id].loadedHandle,
						{ .imageSubresource = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
												.mipLevel = 0,
												.baseArrayLayer = 0,
												.layerCount = 1 },
						  .imageExtent = { .width = image.width, .height = image.height, .depth = 1 } },
						VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
						VK_ACCESS_SHADER_READ_BIT);
					return false;
				}
				break;
		}
		return false;
	}
} // namespace vanadium::graphics
