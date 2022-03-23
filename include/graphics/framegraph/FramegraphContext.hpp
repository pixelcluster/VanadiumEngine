#pragma once

#include <concepts>
#include <graphics/framegraph/RenderTargetSurface.hpp>
#include <graphics/util/GPUDescriptorSetAllocator.hpp>
#include <graphics/util/GPUResourceAllocator.hpp>
#include <graphics/util/GPUTransferManager.hpp>
#include <robin_hood.h>

#include <graphics/framegraph/QueueBarrierGenerator.hpp>

namespace vanadium::graphics {

	class FramegraphNode;

	struct FramegraphNodeBufferUsage {
		std::vector<NodeBufferSubresourceAccess> subresourceAccesses;
		bool writes;
		VkBufferUsageFlags usageFlags;
	};

	struct FramegraphNodeImageUsage {
		std::vector<NodeImageSubresourceAccess> subresourceAccesses;
		VkImageUsageFlags usageFlags;
		bool writes;
		std::vector<ImageResourceViewInfo> viewInfos;
	};

	struct FramegraphBufferCreationParameters {
		VkDeviceSize size;
		VkBufferCreateFlags flags;
	};

	struct FramegraphBufferResource {
		FramegraphBufferCreationParameters creationParameters;
		BufferResourceHandle resourceHandle;

		VkBufferUsageFlags usageFlags;
	};

	using FramegraphBufferHandle = SlotmapHandle;

	struct FramegraphImageCreationParameters {
		VkImageCreateFlags flags;
		VkImageType imageType;
		VkFormat format;
		VkExtent3D extent;
		uint32_t mipLevels;
		uint32_t arrayLayers;
		VkSampleCountFlagBits samples;
		VkImageTiling tiling;
	};

	struct FramegraphImageResource {
		FramegraphImageCreationParameters creationParameters;
		VkImageUsageFlags usage;
		ImageResourceHandle resourceHandle;
	};

	using FramegraphImageHandle = SlotmapHandle;

	struct FramegraphNodeInfo {
		FramegraphNode* node;

		robin_hood::unordered_map<FramegraphImageHandle, std::vector<ImageResourceViewInfo>> resourceViewInfos;
		std::vector<ImageResourceViewInfo> swapchainResourceViewInfos;
	};

	struct FramegraphNodeContext {
		uint32_t frameIndex;

		RenderTargetSurface* targetSurface;
		std::vector<VkImageView> targetImageViews;

		robin_hood::unordered_map<FramegraphImageHandle, std::vector<VkImageView>> resourceImageViews;
	};

	class FramegraphContext {
	  public:
		FramegraphContext() {}

		void create(DeviceContext* context, GPUResourceAllocator* resourceAllocator,
					GPUDescriptorSetAllocator* descriptorSetAllocator, GPUTransferManager* transferManager,
					RenderTargetSurface* targetSurface);

		template <std::derived_from<FramegraphNode> T, typename... Args>
		requires(std::constructible_from<T, Args...>) T* appendNode(Args... constructorArgs);

		void setupResources();

		FramegraphBufferHandle declareTransientBuffer(FramegraphNode* creator,
													  const FramegraphBufferCreationParameters& parameters,
													  const FramegraphNodeBufferUsage& usage);
		FramegraphImageHandle declareTransientImage(FramegraphNode* creator,
													const FramegraphImageCreationParameters& parameters,
													const FramegraphNodeImageUsage& usage);
		FramegraphBufferHandle declareImportedBuffer(FramegraphNode* creator, BufferResourceHandle handle,
													 const FramegraphNodeBufferUsage& usage);
		FramegraphImageHandle declareImportedImage(FramegraphNode* creator, ImageResourceHandle handle,
												   const FramegraphNodeImageUsage& usage);

		// These functions depend on the fact that all references are inserted in execution order!
		void declareReferencedBuffer(FramegraphNode* user, FramegraphBufferHandle handle,
									 const FramegraphNodeBufferUsage& usage);
		void declareReferencedImage(FramegraphNode* user, FramegraphImageHandle handle,
									const FramegraphNodeImageUsage& usage);

		void declareReferencedSwapchainImage(FramegraphNode* user, const FramegraphNodeImageUsage& usage);

		void invalidateBuffer(FramegraphBufferHandle handle, BufferResourceHandle newHandle);
		void invalidateImage(FramegraphImageHandle handle, ImageResourceHandle newHandle);

		DeviceContext* gpuContext() { return m_gpuContext; }
		GPUResourceAllocator* resourceAllocator() { return m_resourceAllocator; }
		GPUDescriptorSetAllocator* descriptorSetAllocator() { return m_descriptorSetAllocator; }
		GPUTransferManager* transferManager() { return m_transferManager; }

		FramegraphBufferResource bufferResource(FramegraphBufferHandle handle) const;
		FramegraphImageResource imageResource(FramegraphImageHandle handle) const;

		void recreateBufferResource(FramegraphBufferHandle handle,
									const FramegraphBufferCreationParameters& parameters);
		void recreateImageResource(FramegraphImageHandle handle, const FramegraphImageCreationParameters& parameters);

		VkBuffer nativeBufferHandle(FramegraphBufferHandle handle);
		VkImage nativeImageHandle(FramegraphImageHandle handle);

		VkImageView imageView(FramegraphNode* node, FramegraphImageHandle handle, size_t index);
		VkImageView targetImageView(FramegraphNode* node, uint32_t index);

		VkBufferUsageFlags bufferUsageFlags(FramegraphBufferHandle handle);
		VkImageUsageFlags imageUsageFlags(FramegraphImageHandle handle);

		VkImageUsageFlags targetImageUsageFlags() const { return m_targetImageUsageFlags; }

		VkCommandBuffer recordFrame(uint32_t frameIndex);

		void handleSwapchainResize(uint32_t width, uint32_t height);

		void destroy();

	  private:
		void createBuffer(FramegraphBufferHandle handle);
		void createImage(FramegraphImageHandle handle);

		void updateDependencyInfo();
		void updateBarriers();

		DeviceContext* m_gpuContext;
		GPUResourceAllocator* m_resourceAllocator;
		GPUDescriptorSetAllocator* m_descriptorSetAllocator;
		GPUTransferManager* m_transferManager;
		RenderTargetSurface* m_targetSurface;

		QueueBarrierGenerator m_barrierGenerator;

		VkCommandPool m_frameCommandPools[frameInFlightCount];
		VkCommandBuffer m_frameCommandBuffers[frameInFlightCount];

		std::vector<FramegraphNodeInfo> m_nodes;

		Slotmap<FramegraphBufferResource> m_buffers;
		Slotmap<FramegraphImageResource> m_images;

		std::vector<FramegraphBufferHandle> m_transientBuffers;
		std::vector<FramegraphImageHandle> m_transientImages;

		bool m_firstFrameFlag = true;

		VkImageUsageFlags m_targetImageUsageFlags = 0;
	};

	template <std::derived_from<FramegraphNode> T, typename... Args>
	requires(std::constructible_from<T, Args...>) inline T* FramegraphContext::appendNode(Args... constructorArgs) {
		m_nodes.push_back({ .node = new T(constructorArgs...) });
		// FramegraphNode might not be defined at this point, but T will contain all of its methods
		reinterpret_cast<T*>(m_nodes.back().node)->create(this);
		return reinterpret_cast<T*>(m_nodes.back().node);
	}
} // namespace vanadium::graphics