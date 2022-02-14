#pragma once

#include <modules/gpu/VGPUDescriptorSetAllocator.hpp>
#include <modules/gpu/VGPUResourceAllocator.hpp>
#include <modules/gpu/transfer/VGPUTransferManager.hpp>
#include <concepts>

class VFramegraphNode;

struct VFramegraphNodeBufferUsage {
	VkPipelineStageFlags pipelineStages;
	VkAccessFlags accessTypes;

	VkDeviceSize offset;
	VkDeviceSize size;
	bool writes;
	VkBufferUsageFlags usageFlags;
};

struct VFramegraphBufferBarrier {
	size_t nodeIndex;
	VkPipelineStageFlags srcStages;
	VkPipelineStageFlags dstStages;
	VkBufferMemoryBarrier barrier;
};

struct VFramegraphNodeBufferAccess {
	size_t usingNodeIndex;

	VkPipelineStageFlags stageFlags;
	VkAccessFlags accessFlags;

	VkDeviceSize offset;
	VkDeviceSize size;
	std::optional<VFramegraphBufferBarrier> barrier;
};
inline bool operator<(const VFramegraphNodeBufferAccess& one, const VFramegraphNodeBufferAccess& other) {
	return one.usingNodeIndex < other.usingNodeIndex;
};

struct VFramegraphNodeImageUsage {
	VkPipelineStageFlags pipelineStages;
	VkAccessFlags accessTypes;
	VkImageLayout startLayout;
	VkImageLayout finishLayout;
	VkImageUsageFlags usageFlags;
	VkImageSubresourceRange subresourceRange;
	bool writes;
	std::optional<VImageResourceViewInfo> viewInfo;
};

struct VFramegraphImageBarrier {
	size_t nodeIndex;
	VkPipelineStageFlags srcStages;
	VkPipelineStageFlags dstStages;
	VkImageMemoryBarrier barrier;
};

struct VFramegraphNodeImageAccess {
	size_t usingNodeIndex;

	VkPipelineStageFlags stageFlags;
	VkAccessFlags accessFlags;
	VkImageLayout startLayout;
	VkImageLayout finishLayout;

	VkImageSubresourceRange subresourceRange;

	std::optional<VFramegraphImageBarrier> barrier;
};
inline bool operator<(const VFramegraphNodeImageAccess& one, const VFramegraphNodeImageAccess& other) {
	return one.usingNodeIndex < other.usingNodeIndex;
};

struct VFramegraphBufferCreationParameters {
	VkDeviceSize size;
	VkBufferCreateFlags flags;
};

struct VFramegraphBufferResource {
	VFramegraphBufferCreationParameters creationParameters;
	VBufferResourceHandle resourceHandle;

	VkBufferUsageFlags usageFlags;
	std::vector<VFramegraphNodeBufferAccess> modifications;
	std::vector<VFramegraphNodeBufferAccess> reads;
};

using VFramegraphBufferHandle = VSlotmapHandle<VFramegraphBufferResource>;

struct VFramegraphImageCreationParameters {
	VkImageCreateFlags flags;
	VkImageType imageType;
	VkFormat format;
	VkExtent3D extent;
	uint32_t mipLevels;
	uint32_t arrayLayers;
	VkSampleCountFlagBits samples;
	VkImageTiling tiling;
};

struct VFramegraphImageResource {
	VFramegraphImageCreationParameters creationParameters;
	VkImageUsageFlags usage;
	VImageResourceHandle resourceHandle;

	std::vector<VFramegraphNodeImageAccess> modifications;
	std::vector<VFramegraphNodeImageAccess> reads;
};

using VFramegraphImageHandle = VSlotmapHandle<VFramegraphImageResource>;

struct VFramegraphNodeInfo {
	VFramegraphNode* node;

	std::unordered_map<VFramegraphImageHandle, VImageResourceViewInfo> resourceViewInfos;
	std::optional<VImageResourceViewInfo> swapchainResourceViewInfo;
};

struct VFramegraphBarrierStages {
	VkPipelineStageFlags src;
	VkPipelineStageFlags dst;
};

struct VFramegraphBufferAccessMatch {
	size_t matchIndex;
	bool isPartial;
	size_t offset;
	size_t size;
};

struct VFramegraphImageAccessMatch {
	size_t matchIndex;
	bool isPartial;
	VkImageSubresourceRange range;
};

struct VFramegraphNodeContext {
	uint32_t frameIndex;
	uint32_t imageIndex;
	VkImage swapchainImage;

	VkImageView swapchainImageView;
	std::unordered_map<VFramegraphImageHandle, VkImageView> resourceImageViews;
};

class VFramegraphContext {
  public:
	VFramegraphContext() {}

	void create(VGPUContext* context, VGPUResourceAllocator* resourceAllocator,
				VGPUDescriptorSetAllocator* descriptorSetAllocator, VGPUTransferManager* transferManager);

	template <std::derived_from<VFramegraphNode> T, typename... Args>
	requires(std::constructible_from<T, Args...>) T* appendNode(Args... constructorArgs);

	void setupResources();

	VFramegraphBufferHandle declareTransientBuffer(VFramegraphNode* creator,
														const VFramegraphBufferCreationParameters& parameters,
														const VFramegraphNodeBufferUsage& usage);
	VFramegraphImageHandle declareTransientImage(VFramegraphNode* creator,
													  const VFramegraphImageCreationParameters& parameters,
													  const VFramegraphNodeImageUsage& usage);
	VFramegraphBufferHandle declareImportedBuffer(VFramegraphNode* creator, VBufferResourceHandle handle,
							   const VFramegraphNodeBufferUsage& usage);
	VFramegraphImageHandle declareImportedImage(VFramegraphNode* creator, VImageResourceHandle handle,
							  const VFramegraphNodeImageUsage& usage);

	// These functions depend on the fact that all references are inserted in execution order!
	void declareReferencedBuffer(VFramegraphNode* user, VFramegraphBufferHandle handle,
								 const VFramegraphNodeBufferUsage& usage);
	void declareReferencedImage(VFramegraphNode* user, VFramegraphImageHandle handle,
								const VFramegraphNodeImageUsage& usage);

	void declareReferencedSwapchainImage(VFramegraphNode* user, const VFramegraphNodeImageUsage& usage);

	void invalidateBuffer(VFramegraphBufferHandle handle, VBufferResourceHandle newHandle);
	void invalidateImage(VFramegraphImageHandle handle, VImageResourceHandle newHandle);

	VGPUContext* gpuContext() { return m_gpuContext; }
	VGPUResourceAllocator* resourceAllocator() { return m_resourceAllocator; }
	VGPUDescriptorSetAllocator* descriptorSetAllocator() { return m_descriptorSetAllocator; }
	VGPUTransferManager* transferManager() { return m_transferManager; }

	VFramegraphBufferResource bufferResource(VFramegraphBufferHandle handle) const;
	VFramegraphImageResource imageResource(VFramegraphImageHandle handle) const;

	void recreateBufferResource(VFramegraphBufferHandle handle, const VFramegraphBufferCreationParameters& parameters);
	void recreateImageResource(VFramegraphImageHandle handle, const VFramegraphImageCreationParameters& parameters);

	VkBuffer nativeBufferHandle(VFramegraphBufferHandle handle);
	VkImage nativeImageHandle(VFramegraphImageHandle handle);

	VkImageView swapchainImageView(VFramegraphNode* node, uint32_t index);

	VkBufferUsageFlags bufferUsageFlags(VFramegraphBufferHandle handle);
	VkImageUsageFlags imageUsageFlags(VFramegraphImageHandle handle);

	VkImageUsageFlags swapchainImageUsageFlags() const { return m_swapchainImageUsageFlags; }

	VkCommandBuffer recordFrame(const AcquireResult& result);

	void handleSwapchainResize(uint32_t width, uint32_t height);

	void destroy();

  private:
	void createBuffer(VFramegraphBufferHandle handle);
	void createImage(VFramegraphImageHandle handle);

	void addUsage(VFramegraphBufferHandle handle, size_t nodeIndex, std::vector<VFramegraphNodeBufferAccess>& modifications,
				  std::vector<VFramegraphNodeBufferAccess>& reads, const VFramegraphNodeBufferUsage& usage);
	void addUsage(VFramegraphImageHandle handle, size_t nodeIndex, std::vector<VFramegraphNodeImageAccess>& modifications,
				  std::vector<VFramegraphNodeImageAccess>& reads, const VFramegraphNodeImageUsage& usage);
	void addUsage(size_t nodeIndex, std::vector<VFramegraphNodeImageAccess>& modifications,
				  std::vector<VFramegraphNodeImageAccess>& reads, const VFramegraphNodeImageUsage& usage);

	void updateDependencyInfo();
	void updateBarriers(uint32_t imageIndex);

	void emitBarriersForRead(VkBuffer buffer, std::vector<VFramegraphNodeBufferAccess>& modifications,
							 const VFramegraphNodeBufferAccess& read);
	void emitBarriersForRead(VkImage image, std::vector<VFramegraphNodeImageAccess>& modifications,
							 const VFramegraphNodeImageAccess& read);

	std::optional<VFramegraphBufferAccessMatch> findLastModification(
		std::vector<VFramegraphNodeBufferAccess>& modifications, const VFramegraphNodeBufferAccess& read);
	std::optional<VFramegraphImageAccessMatch> findLastModification(
		std::vector<VFramegraphNodeImageAccess>& modifications, const VFramegraphNodeImageAccess& read);

	void emitBarrier(VkBuffer buffer, VFramegraphNodeBufferAccess& modification,
					 const VFramegraphNodeBufferAccess& read, const VFramegraphBufferAccessMatch& match);
	void emitBarrier(VkImage image, VFramegraphNodeImageAccess& modification, const VFramegraphNodeImageAccess& read,
					 const VFramegraphImageAccessMatch& match);

	VGPUContext* m_gpuContext;
	VGPUResourceAllocator* m_resourceAllocator;
	VGPUDescriptorSetAllocator* m_descriptorSetAllocator;
	VGPUTransferManager* m_transferManager;

	VkCommandBuffer m_frameCommandBuffers[frameInFlightCount];

	std::vector<VFramegraphNodeInfo> m_nodes;

	VSlotmap<VFramegraphBufferResource> m_buffers;
	VSlotmap<VFramegraphImageResource> m_images;

	std::vector<VFramegraphBufferHandle> m_transientBuffers;
	std::vector<VFramegraphImageHandle> m_transientImages;

	bool m_firstFrameFlag = true;

	std::vector<VkImageMemoryBarrier> m_initImageMemoryBarriers;
	VFramegraphBarrierStages m_initBarrierStages = {};

	std::vector<VkImageMemoryBarrier> m_frameStartImageMemoryBarriers;
	std::vector<VkBufferMemoryBarrier> m_frameStartBufferMemoryBarriers;
	VFramegraphBarrierStages m_frameStartBarrierStages = {};

	std::vector<std::vector<VkBufferMemoryBarrier>> m_nodeBufferMemoryBarriers;
	std::vector<std::vector<VkImageMemoryBarrier>> m_nodeImageMemoryBarriers;
	std::vector<VFramegraphBarrierStages> m_nodeBarrierStages;

	std::vector<VFramegraphNodeImageAccess> m_swapchainImageModifications;
	std::vector<VFramegraphNodeImageAccess> m_swapchainImageReads;
	VkImageUsageFlags m_swapchainImageUsageFlags = 0;
	std::vector<std::unordered_map<VImageResourceViewInfo, VkImageView>> m_swapchainImageViews = { {} };
};

template <std::derived_from<VFramegraphNode> T, typename... Args>
requires(std::constructible_from<T, Args...>) inline T* VFramegraphContext::appendNode(
	Args... constructorArgs) {
	m_nodes.push_back({ .node = new T(constructorArgs...) });
	//VFramegraphNode might not be defined at this point, but T will contain all of its methods
	reinterpret_cast<T*>(m_nodes.back().node)->create(this);
	return reinterpret_cast<T*>(m_nodes.back().node);
}
