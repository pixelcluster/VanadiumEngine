#pragma once

#include <graphics/util/GPUResourceAllocator.hpp>
#include <graphics/util/GPUTransferManager.hpp>

#include <stb_image.h>

namespace vanadium::graphics {

	ImageResourceHandle loadTexture(const char* filename, GPUResourceAllocator* allocator,
									GPUTransferManager* transferManager, VkImageUsageFlags usageFlags,
									VkPipelineStageFlags usingStageFlags, VkAccessFlags usingAccessFlags,
									VkImageLayout usingImageLayout) {
		int channels = 4;

		int imageWidth, imageHeight;

		void* data = stbi_load(filename, &imageWidth, &imageHeight, &channels, channels);

		VkImageCreateInfo imageCreateInfo = { .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
											  .imageType = VK_IMAGE_TYPE_2D,
											  .format = VK_FORMAT_R8G8B8A8_SRGB,
											  .extent = { .width = static_cast<uint32_t>(imageWidth),
														  .height = static_cast<uint32_t>(imageHeight),
														  .depth = 1U },
											  .mipLevels = 1U,
											  .arrayLayers = 1U,
											  .samples = VK_SAMPLE_COUNT_1_BIT,
											  .tiling = VK_IMAGE_TILING_OPTIMAL,
											  .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | usageFlags,
											  .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
											  .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED };

		ImageResourceHandle handle = allocator->createImage(imageCreateInfo, {}, { .deviceLocal = true });

		transferManager->submitImageTransfer(handle,
											 { .imageSubresource = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
																	 .mipLevel = 0,
																	 .baseArrayLayer = 0,
																	 .layerCount = 1 },
											   .imageExtent = { .width = static_cast<uint32_t>(imageWidth),
																.height = static_cast<uint32_t>(imageHeight),
																.depth = 1U } },
											 data, imageWidth * imageHeight * 4, usingStageFlags, usingAccessFlags,
											 usingImageLayout);

		stbi_image_free(data);
		return handle;
	}

} // namespace vanadium::graphics