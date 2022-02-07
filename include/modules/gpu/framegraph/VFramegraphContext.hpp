#pragma once

#include <modules/gpu/VGPUResourceAllocator.hpp>
#include <string_view>
#include <helper/VFramegraphStringAllocator.hpp>
#include <modules/gpu/VGPUDescriptorSetAllocator.hpp>
#include <modules/gpu/transfer/VGPUTransferManager.hpp>

using VFramegraphString = std::basic_string<char, std::char_traits<char>, VFramegraphStringAllocator<char>>;

class VFramegraphNode;

struct VFramegraphBufferCreationParameters {
	VkDeviceSize size;
	VkBufferCreateFlags flags;
};

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

struct VFramegraphNodeContext {
	uint32_t frameIndex;
	uint32_t imageIndex;
	VkImage swapchainImage;

	VkImageView swapchainImageView;
	std::unordered_map<std::string, VkImageView> resourceImageViews;
};

struct VFramegraphNodeBufferUsage {
	VkPipelineStageFlags pipelineStages;
	VkAccessFlags accessTypes;

	VkDeviceSize offset;
	VkDeviceSize size;
	bool writes;
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
	VkImageAspectFlags aspectFlags;
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

struct VFramegraphBufferResource {
	VBufferResourceHandle bufferResourceHandle;

	VkBufferUsageFlags usageFlags;
	std::vector<VFramegraphNodeBufferAccess> modifications;
	std::vector<VFramegraphNodeBufferAccess> reads;
};

struct VFramegraphImageResource {
	VImageResourceHandle imageResourceHandle;

	VkImageUsageFlags usageFlags;
	std::vector<VFramegraphNodeImageAccess> modifications;
	std::vector<VFramegraphNodeImageAccess> reads;
};

struct VFramegraphNodeInfo {
	VFramegraphNode* node;

	std::unordered_map<VFramegraphString, VImageResourceViewInfo> resourceViewInfos;
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

class VFramegraphContext {
  public:
	VFramegraphContext() {}

	void create(VGPUContext* context, VGPUResourceAllocator* resourceAllocator, VGPUDescriptorSetAllocator* descriptorSetAllocator, VGPUTransferManager* transferManager);

	template <DerivesFrom<VFramegraphNode> T, typename... Args>
	requires(ConstructibleWith<T, Args...>) VFramegraphNode* appendNode(Args... constructorArgs);

	void setupResources();

	void declareCreatedBuffer(VFramegraphNode* creator, const std::string_view& name,
							  const VFramegraphBufferCreationParameters& parameters,
							  const VFramegraphNodeBufferUsage& usage);
	void declareCreatedImage(VFramegraphNode* creator, const std::string_view& name,
							 const VFramegraphImageCreationParameters& parameters,
							 const VFramegraphNodeImageUsage& usage);
	void declareImportedBuffer(VFramegraphNode* creator, const std::string_view& name, VBufferResourceHandle handle,
							   const VFramegraphNodeBufferUsage& usage);
	void declareImportedImage(VFramegraphNode* creator, const std::string_view& name, VImageResourceHandle handle,
							  const VFramegraphNodeImageUsage& usage);

	// These functions depend on the fact that all references are inserted in execution order!
	void declareReferencedBuffer(VFramegraphNode* user, const std::string_view& name,
								 const VFramegraphNodeBufferUsage& usage);
	void declareReferencedImage(VFramegraphNode* user, const std::string_view& name,
								const VFramegraphNodeImageUsage& usage);

	void declareReferencedSwapchainImage(VFramegraphNode* user, const VFramegraphNodeImageUsage& usage);

	void invalidateBuffer(const std::string_view& name, VBufferResourceHandle newHandle);
	void invalidateImage(const std::string_view& name, VImageResourceHandle newHandle);

	VGPUContext* gpuContext() { return m_gpuContext; }
	VGPUResourceAllocator* resourceAllocator() { return m_resourceAllocator; }
	VGPUDescriptorSetAllocator* descriptorSetAllocator() { return m_descriptorSetAllocator; }
	VGPUTransferManager* transferManager() { return m_transferManager; }

	VFramegraphBufferResource bufferResource(const std::string_view& name) const;
	VFramegraphImageResource imageResource(const std::string_view& name) const;

	void recreateBufferResource(const std::string_view& name, const VFramegraphBufferCreationParameters& parameters);
	void recreateImageResource(const std::string_view& name, const VFramegraphImageCreationParameters& parameters);

	VkBuffer nativeBufferHandle(const std::string_view& name);
	VkImage nativeImageHandle(const std::string_view& name);

	VkImageView swapchainImageView(VFramegraphNode* node, uint32_t index);

	VkBufferUsageFlags bufferUsageFlags(const std::string_view& name);
	VkImageUsageFlags imageUsageFlags(const std::string_view& name);

	VkImageUsageFlags swapchainImageUsageFlags() const { return m_swapchainImageUsageFlags; }

	VkCommandBuffer recordFrame(const AcquireResult& result);

	void handleSwapchainResize(uint32_t width, uint32_t height);

	void destroy();

  private:
	void createBuffer(const std::string_view& name);
	void createImage(const std::string_view& name);

	void addUsage(size_t nodeIndex, std::vector<VFramegraphNodeBufferAccess>& modifications,
				  std::vector<VFramegraphNodeBufferAccess>& reads, const VFramegraphNodeBufferUsage& usage);
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

	VFramegraphStringAllocator<char> m_stringAllocator = VFramegraphStringAllocator<char>(16384);

	VkCommandPool m_frameCommandPools[frameInFlightCount];
	VkCommandBuffer m_frameCommandBuffers[frameInFlightCount];

	std::vector<VFramegraphNodeInfo> m_nodes;

	std::unordered_map<VFramegraphString, VFramegraphBufferResource> m_buffers;
	std::unordered_map<VFramegraphString, VFramegraphImageResource> m_images;

	std::unordered_map<VFramegraphString, VFramegraphBufferCreationParameters> m_createdBufferParameters;
	std::unordered_map<VFramegraphString, VFramegraphImageCreationParameters> m_createdImageParameters;

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

template <DerivesFrom<VFramegraphNode> T, typename... Args>
requires(ConstructibleWith<T, Args...>) inline VFramegraphNode* VFramegraphContext::appendNode(
	Args... constructorArgs) {
	m_nodes.push_back({ .node = new T(constructorArgs...) });
	return m_nodes.back().node;
}
