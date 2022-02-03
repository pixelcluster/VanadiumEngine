#pragma once

#include <modules/gpu/VGPUResourceAllocator.hpp>
#include <string_view>

class VFramegraphNode;

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
	VkBufferUsageFlags usageFlags;

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

	std::unordered_map<std::string, VImageResourceViewInfo> resourceViewInfos;
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

	void create(VGPUContext* context, VGPUResourceAllocator* resourceAllocator);

	template <DerivesFrom<VFramegraphNode> T, typename... Args>
	requires(ConstructibleWith<T, Args...>) VFramegraphNode* appendNode(Args... constructorArgs);

	void setupResources();

	void declareCreatedBuffer(VFramegraphNode* creator, const std::string_view& name, VBufferResourceHandle handle,
							  const VFramegraphNodeBufferUsage& usage);
	void declareCreatedImage(VFramegraphNode* creator, const std::string_view& name, VImageResourceHandle handle,
							 const VFramegraphNodeImageUsage& usage);

	// These functions depend on the fact that all references are inserted in execution order!
	void declareReferencedBuffer(VFramegraphNode* user, const std::string_view& name,
								 const VFramegraphNodeBufferUsage& usage);
	void declareReferencedImage(VFramegraphNode* user, const std::string_view& name,
								const VFramegraphNodeImageUsage& usage);

	void declareReferencedSwapchainImage(VFramegraphNode* user, const VFramegraphNodeImageUsage& usage);

	void invalidateBuffer(const std::string& name, VBufferResourceHandle newHandle);
	void invalidateImage(const std::string& name, VImageResourceHandle newHandle);

	VGPUContext* gpuContext() { return m_gpuContext; }
	VGPUResourceAllocator* resourceAllocator() { return m_resourceAllocator; }

	const VFramegraphBufferResource bufferResource(const std::string& name) const;
	const VFramegraphImageResource imageResource(const std::string& name) const;

	VkBuffer nativeBufferHandle(const std::string& name) {
		return m_resourceAllocator->nativeBufferHandle(m_buffers[name].bufferResourceHandle);
	}
	VkImage nativeImageHandle(const std::string& name) {
		return m_resourceAllocator->nativeImageHandle(m_images[name].imageResourceHandle);
	}
	VkImageView swapchainImageView(VFramegraphNode* node, uint32_t index);

	VkBufferUsageFlags bufferUsageFlags(const std::string& name) { return m_buffers[name].usageFlags; }
	VkImageUsageFlags imageUsageFlags(const std::string& name) { return m_images[name].usageFlags; }

	VkImageUsageFlags swapchainImageUsageFlags() const { return m_swapchainImageUsageFlags; }

	void executeFrame(const AcquireResult& result, VkSemaphore signalSemaphore);

	void handleSwapchainResize(uint32_t width, uint32_t height);
  private:
	void addUsage(size_t nodeIndex, std::vector<VFramegraphNodeBufferAccess>& modifications,
				  std::vector<VFramegraphNodeBufferAccess>& reads, const VFramegraphNodeBufferUsage& usage);
	void addUsage(size_t nodeIndex, std::vector<VFramegraphNodeImageAccess>& modifications,
				  std::vector<VFramegraphNodeImageAccess>& reads, const VFramegraphNodeImageUsage& usage);

	void updateNodeBarriers();

	std::optional<VFramegraphBufferAccessMatch> findLastModification(
		std::vector<VFramegraphNodeBufferAccess>& modifications, const VFramegraphNodeBufferAccess& read);
	std::optional<VFramegraphImageAccessMatch> findLastModification(
		std::vector<VFramegraphNodeImageAccess>& modifications, const VFramegraphNodeImageAccess& read);

	void emitBarrier(VkBuffer buffer, VFramegraphNodeBufferAccess& modification, const VFramegraphNodeBufferAccess& read);
	void emitBarrier(VkImage image, VFramegraphNodeImageAccess& modification, const VFramegraphNodeImageAccess& read);

	VGPUContext* m_gpuContext;
	VGPUResourceAllocator* m_resourceAllocator;

	std::vector<VFramegraphNodeInfo> m_nodes;

	std::vector<std::vector<VkBufferMemoryBarrier>> m_nodeBufferMemoryBarriers;
	std::vector<std::vector<VkImageMemoryBarrier>> m_nodeImageMemoryBarriers;
	std::vector<VFramegraphBarrierStages> m_nodeBarrierStages;

	std::unordered_map<std::string, VFramegraphBufferResource> m_buffers;
	std::unordered_map<std::string, VFramegraphImageResource> m_images;

	std::vector<VFramegraphNodeImageAccess> m_swapchainImageModifications;
	std::vector<VFramegraphNodeImageAccess> m_swapchainImageReads;
	VkImageUsageFlags m_swapchainImageUsageFlags;
	std::vector<std::unordered_map<VImageResourceViewInfo, VkImageView>> m_swapchainImageViews = { {} };
};

template <DerivesFrom<VFramegraphNode> T, typename... Args>
requires(ConstructibleWith<T, Args...>) inline VFramegraphNode* VFramegraphContext::appendNode(
	Args... constructorArgs) {
	m_nodes.push_back({ .node = new T(constructorArgs...) });
	return m_nodes.back().node;
}
