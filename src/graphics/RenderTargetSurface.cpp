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
#include <Debug.hpp>
#include <graphics/RenderTargetSurface.hpp>
#include <graphics/helper/DebugHelper.hpp>
#include <volk.h>

namespace vanadium::graphics {
	RenderTargetSurface::RenderTargetSurface(DeviceContext* context, VkFormat imageFormat) : m_context(context) {
		m_properties = {
			.format = imageFormat
		};
	}

	void RenderTargetSurface::create(const std::vector<VkImage>& swapchainImages,
									 const RenderTargetSurfaceProperties& properties) {
		m_properties = properties;
		m_images = swapchainImages;

		for (auto& viewMap : m_imageViews) {
			for (auto& view : viewMap) {
				vkDestroyImageView(m_context->device(), view.second, nullptr);
			}
		}

		if (!m_imageViews.empty())
			m_imageViews.resize(m_images.size(), *m_imageViews.begin());
		else
			m_imageViews.resize(m_images.size());

		for (size_t i = 0; i < m_images.size(); ++i) {
			if constexpr (vanadiumGPUDebug) {
				setObjectName(m_context->device(), VK_OBJECT_TYPE_IMAGE, m_images[i],
							  "Swapchain image for index " + std::to_string(i));
			}

			auto& imageViewMap = m_imageViews[i];
			for (auto& viewInfo : imageViewMap) {
				VkImageViewCreateInfo createInfo = { .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
													 .flags = viewInfo.first.flags,
													 .image = m_images[i],
													 .viewType = viewInfo.first.viewType,
													 .format = m_properties.format,
													 .components = viewInfo.first.components,
													 .subresourceRange = viewInfo.first.subresourceRange };
				vkCreateImageView(m_context->device(), &createInfo, nullptr, &viewInfo.second);

				if constexpr (vanadiumGPUDebug) {
					setObjectName(m_context->device(), VK_OBJECT_TYPE_IMAGE_VIEW, viewInfo.second,
								  "Swapchain image view for index " + std::to_string(i));
				}
			}
		}
	}

	void RenderTargetSurface::addRequestedView(const ImageResourceViewInfo& info) {
		for (size_t i = 0; i < m_images.size(); ++i) {
			if(m_imageViews[i].find(info) != m_imageViews[i].end()) {
				continue;
			}
			VkImageView view;
			VkImageViewCreateInfo createInfo = { .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
												 .flags = info.flags,
												 .image = m_images[i],
												 .viewType = info.viewType,
												 .format = m_properties.format,
												 .components = info.components,
												 .subresourceRange = info.subresourceRange };
			vkCreateImageView(m_context->device(), &createInfo, nullptr, &view);
			m_imageViews[i].insert(robin_hood::pair<const ImageResourceViewInfo, VkImageView>(info, view));
		}
	}

	void RenderTargetSurface::destroy() {
		for (auto& views : m_imageViews) {
			for (auto& view : views) {
				vkDestroyImageView(m_context->device(), view.second, nullptr);
			}
		}
	}
} // namespace vanadium::graphics
