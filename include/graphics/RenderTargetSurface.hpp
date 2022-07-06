#pragma once

#include <util/Vector.hpp>
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
		SimpleVector<VkFramebuffer> m_framebuffers;
	};

	class RenderTargetSurface {
	  public:
		RenderTargetSurface(DeviceContext* context, VkFormat imageFormat);
		void create(const SimpleVector<VkImage>& swapchainImages, const RenderTargetSurfaceProperties& properties);

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

		SimpleVector<VkImage> m_images;
		SimpleVector<robin_hood::unordered_map<ImageResourceViewInfo, VkImageView>> m_imageViews = { {} };
		robin_hood::unordered_map<RenderPassSignature, RenderTargetFramebufferContainer> m_framebufferContainers;
		RenderTargetSurfaceProperties m_properties;
	};

} // namespace vanadium::graphics