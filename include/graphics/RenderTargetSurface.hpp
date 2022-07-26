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

#include <vector>
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#include <graphics/DeviceContext.hpp>
#include <graphics/RenderPassSignature.hpp>
#include <graphics/util/GPUResourceAllocator.hpp>

namespace vanadium::graphics {

	struct RenderTargetSurfaceProperties {
		uint32_t width;
		uint32_t height;
		VkFormat format;
	};

	struct RenderTargetFramebufferContainer {
		uint64_t revisionIndex;
		std::vector<VkFramebuffer> m_framebuffers;
	};

	class RenderTargetSurface {
	  public:
		RenderTargetSurface(DeviceContext* context, VkFormat imageFormat);
		void create(const std::vector<VkImage>& swapchainImages, const RenderTargetSurfaceProperties& properties);

		void setTargetImageIndex(uint32_t index) { m_currentTargetIndex = index; }
		uint32_t currentTargetIndex() const { return m_currentTargetIndex; }
		uint32_t currentImageCount() const { return static_cast<uint32_t>(m_images.size()); }

		void addRequestedView(const ImageResourceViewInfo& info);

		const RenderTargetSurfaceProperties& properties() const { return m_properties; }
		VkImageView currentTargetView(const ImageResourceViewInfo& info) {
			return m_imageViews[m_currentTargetIndex][info];
		}
		VkImageView targetView(uint32_t index, const ImageResourceViewInfo& info) { return m_imageViews[index][info]; }
		VkImage currentTargetImage() { return m_images[m_currentTargetIndex]; }

		void destroy();

	  private:
		DeviceContext* m_context;

		uint32_t m_currentTargetIndex;

		std::vector<VkImage> m_images;
		std::vector<robin_hood::unordered_map<ImageResourceViewInfo, VkImageView>> m_imageViews = { {} };
		robin_hood::unordered_map<RenderPassSignature, RenderTargetFramebufferContainer> m_framebufferContainers;
		RenderTargetSurfaceProperties m_properties;
	};

} // namespace vanadium::graphics
