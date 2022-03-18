#pragma once

#include <vector>
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#include <graphics/DeviceContext.hpp>
#include <graphics/util/GPUResourceAllocator.hpp>

namespace vanadium::graphics {

	struct RenderTargetSurfaceProperties {
		uint32_t width;
		uint32_t height;
		VkFormat format;
	};

	class RenderTargetSurface {
	  public:
		RenderTargetSurface(DeviceContext* context);
		void create(const std::vector<VkImage>& swapchainImages, const RenderTargetSurfaceProperties& properties);

		void setTargetImageIndex(uint32_t index) { m_currentTargetIndex = index; }
		void addRequestedView(const ImageResourceViewInfo& info);

		const RenderTargetSurfaceProperties& properties() const { return m_properties; }
		VkImageView currentTargetView(const ImageResourceViewInfo& info) {
			return m_imageViews[m_currentTargetIndex][info];
		}
		VkImage currentTargetImage() { return m_images[m_currentTargetIndex]; }

		void destroy();

	  private:
		DeviceContext* m_context;

		uint32_t m_currentTargetIndex;

		std::vector<VkImage> m_images;
		std::vector<robin_hood::unordered_map<ImageResourceViewInfo, VkImageView>> m_imageViews = { {} };
		RenderTargetSurfaceProperties m_properties;
	};

} // namespace vanadium::graphics