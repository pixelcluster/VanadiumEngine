#pragma once

#include <concepts>
#include <graphics/framegraph/RenderTargetSurface.hpp>
#include <graphics/util/GPUDescriptorSetAllocator.hpp>
#include <graphics/util/GPUResourceAllocator.hpp>
#include <graphics/util/GPUTransferManager.hpp>
#include <robin_hood.h>

namespace vanadium::graphics {

	class FramegraphNode;

	struct FramegraphNodeBufferUsage {
		VkPipelineStageFlags pipelineStages;
		VkAccessFlags accessTypes;

		VkDeviceSize offset;
		VkDeviceSize size;
		bool writes;
		VkBufferUsageFlags usageFlags;
	};

	struct FramegraphBufferBarrier {
		size_t nodeIndex;
		VkPipelineStageFlags srcStages;
		VkPipelineStageFlags dstStages;
		VkBufferMemoryBarrier barrier;
	};

	struct FramegraphNodeBufferAccess {
		size_t usingNodeIndex;

		VkPipelineStageFlags stageFlags;
		VkAccessFlags accessFlags;

		VkDeviceSize offset;
		VkDeviceSize size;
		std::optional<FramegraphBufferBarrier> barrier;
	};
	inline bool operator<(const FramegraphNodeBufferAccess& one, const FramegraphNodeBufferAccess& other) {
		return one.usingNodeIndex < other.usingNodeIndex;
	};

	struct FramegraphNodeImageUsage {
		VkPipelineStageFlags pipelineStages;
		VkAccessFlags accessTypes;
		VkImageLayout startLayout;
		VkImageLayout finishLayout;
		VkImageUsageFlags usageFlags;
		VkImageSubresourceRange subresourceRange;
		bool writes;
		std::optional<ImageResourceViewInfo> viewInfo;
	};

	struct FramegraphImageBarrier {
		size_t nodeIndex;
		VkPipelineStageFlags srcStages;
		VkPipelineStageFlags dstStages;
		VkImageMemoryBarrier barrier;
	};

	struct FramegraphNodeImageAccess {
		size_t usingNodeIndex;

		VkPipelineStageFlags stageFlags;
		VkAccessFlags accessFlags;
		VkImageLayout startLayout;
		VkImageLayout finishLayout;

		VkImageSubresourceRange subresourceRange;

		std::optional<FramegraphImageBarrier> barrier;
	};
	inline bool operator<(const FramegraphNodeImageAccess& one, const FramegraphNodeImageAccess& other) {
		return one.usingNodeIndex < other.usingNodeIndex;
	};

	struct FramegraphBufferCreationParameters {
		VkDeviceSize size;
		VkBufferCreateFlags flags;
	};

	struct FramegraphBufferResource {
		FramegraphBufferCreationParameters creationParameters;
		BufferResourceHandle resourceHandle;

		VkBufferUsageFlags usageFlags;
		std::vector<FramegraphNodeBufferAccess> modifications;
		std::vector<FramegraphNodeBufferAccess> reads;
	};

	using FramegraphBufferHandle = SlotmapHandle<FramegraphBufferResource>;

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

		std::vector<FramegraphNodeImageAccess> modifications;
		std::vector<FramegraphNodeImageAccess> reads;
	};

	using FramegraphImageHandle = SlotmapHandle<FramegraphImageResource>;

	struct FramegraphNodeInfo {
		FramegraphNode* node;

		robin_hood::unordered_map<FramegraphImageHandle, ImageResourceViewInfo> resourceViewInfos;
		std::optional<ImageResourceViewInfo> swapchainResourceViewInfo;
	};

	struct FramegraphBarrierStages {
		VkPipelineStageFlags src;
		VkPipelineStageFlags dst;
	};

	struct FramegraphBufferAccessMatch {
		size_t matchIndex;
		bool isPartial;
		size_t offset;
		size_t size;
	};

	struct FramegraphImageAccessMatch {
		size_t matchIndex;
		bool isPartial;
		VkImageSubresourceRange range;
	};

	struct FramegraphNodeContext {
		uint32_t frameIndex;
		
		RenderTargetSurface* targetSurface;
		VkImageView targetImageView;

		robin_hood::unordered_map<FramegraphImageHandle, VkImageView> resourceImageViews;
	};

	class FramegraphContext {
	  public:
		FramegraphContext() {}

		void create(DeviceContext* context, GPUResourceAllocator* resourceAllocator,
					GPUDescriptorSetAllocator* descriptorSetAllocator, GPUTransferManager* transferManager, RenderTargetSurface* targetSurface);

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

		VkImageView imageView(FramegraphNode* node, FramegraphImageHandle handle);
		VkImageView targetImageView(FramegraphNode* node, uint32_t index);

		VkBufferUsageFlags bufferUsageFlags(FramegraphBufferHandle handle);
		VkImageUsageFlags imageUsageFlags(FramegraphImageHandle handle);

		VkImageUsageFlags swapchainImageUsageFlags() const { return m_swapchainImageUsageFlags; }

		VkCommandBuffer recordFrame(uint32_t frameIndex);

		void handleSwapchainResize(uint32_t width, uint32_t height);

		void destroy();

	  private:
		void createBuffer(FramegraphBufferHandle handle);
		void createImage(FramegraphImageHandle handle);

		void addUsage(FramegraphBufferHandle handle, size_t nodeIndex,
					  std::vector<FramegraphNodeBufferAccess>& modifications,
					  std::vector<FramegraphNodeBufferAccess>& reads, const FramegraphNodeBufferUsage& usage);
		void addUsage(FramegraphImageHandle handle, size_t nodeIndex,
					  std::vector<FramegraphNodeImageAccess>& modifications,
					  std::vector<FramegraphNodeImageAccess>& reads, const FramegraphNodeImageUsage& usage);
		void addUsage(size_t nodeIndex, std::vector<FramegraphNodeImageAccess>& modifications,
					  std::vector<FramegraphNodeImageAccess>& reads, const FramegraphNodeImageUsage& usage);

		void updateDependencyInfo();
		void updateBarriers();

		void emitBarriersForRead(VkBuffer buffer, std::vector<FramegraphNodeBufferAccess>& modifications,
								 const FramegraphNodeBufferAccess& read);
		void emitBarriersForRead(VkImage image, std::vector<FramegraphNodeImageAccess>& modifications,
								 const FramegraphNodeImageAccess& read);

		std::optional<FramegraphBufferAccessMatch> findLastModification(
			std::vector<FramegraphNodeBufferAccess>& modifications, const FramegraphNodeBufferAccess& read);
		std::optional<FramegraphImageAccessMatch> findLastModification(
			std::vector<FramegraphNodeImageAccess>& modifications, const FramegraphNodeImageAccess& read);

		void emitBarrier(VkBuffer buffer, FramegraphNodeBufferAccess& modification,
						 const FramegraphNodeBufferAccess& read, const FramegraphBufferAccessMatch& match);
		void emitBarrier(VkImage image, FramegraphNodeImageAccess& modification, const FramegraphNodeImageAccess& read,
						 const FramegraphImageAccessMatch& match);

		DeviceContext* m_gpuContext;
		GPUResourceAllocator* m_resourceAllocator;
		GPUDescriptorSetAllocator* m_descriptorSetAllocator;
		GPUTransferManager* m_transferManager;
		RenderTargetSurface* m_targetSurface;

		VkCommandPool m_frameCommandPools[frameInFlightCount];
		VkCommandBuffer m_frameCommandBuffers[frameInFlightCount];

		std::vector<FramegraphNodeInfo> m_nodes;

		Slotmap<FramegraphBufferResource> m_buffers;
		Slotmap<FramegraphImageResource> m_images;

		std::vector<FramegraphBufferHandle> m_transientBuffers;
		std::vector<FramegraphImageHandle> m_transientImages;

		bool m_firstFrameFlag = true;

		std::vector<VkImageMemoryBarrier> m_initImageMemoryBarriers;
		FramegraphBarrierStages m_initBarrierStages = {};

		std::vector<VkImageMemoryBarrier> m_frameStartImageMemoryBarriers;
		std::vector<VkBufferMemoryBarrier> m_frameStartBufferMemoryBarriers;
		FramegraphBarrierStages m_frameStartBarrierStages = {};

		std::vector<std::vector<VkBufferMemoryBarrier>> m_nodeBufferMemoryBarriers;
		std::vector<std::vector<VkImageMemoryBarrier>> m_nodeImageMemoryBarriers;
		std::vector<FramegraphBarrierStages> m_nodeBarrierStages;

		std::vector<FramegraphNodeImageAccess> m_swapchainImageModifications;
		std::vector<FramegraphNodeImageAccess> m_swapchainImageReads;
		VkImageUsageFlags m_swapchainImageUsageFlags = 0;
	};

	template <std::derived_from<FramegraphNode> T, typename... Args>
	requires(std::constructible_from<T, Args...>) inline T* FramegraphContext::appendNode(Args... constructorArgs) {
		m_nodes.push_back({ .node = new T(constructorArgs...) });
		// FramegraphNode might not be defined at this point, but T will contain all of its methods
		reinterpret_cast<T*>(m_nodes.back().node)->create(this);
		return reinterpret_cast<T*>(m_nodes.back().node);
	}
} // namespace vanadium::graphics