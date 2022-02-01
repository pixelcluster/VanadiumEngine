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

struct VFramegraphBufferDependency {
	std::string resourceName;
	VkPipelineStageFlags srcStages;
	VkPipelineStageFlags dstStages;
	VkAccessFlags srcAccesses;
	VkAccessFlags dstAccesses;
};

struct VFramegraphImageDependency {
	std::string resourceName;
	VkImageAspectFlags aspectFlags;
	VkPipelineStageFlags srcStages;
	VkPipelineStageFlags dstStages;
	VkAccessFlags srcAccesses;
	VkAccessFlags dstAccesses;
	VkImageLayout oldLayout;
	VkImageLayout newLayout;
};

struct VFramegraphNodeBufferUsage {
	VkPipelineStageFlags pipelineStages;
	VkAccessFlags accessTypes;
	VkBufferUsageFlags usageFlags;
};

struct VFramegraphNodeImageUsage {
	VkPipelineStageFlags pipelineStages;
	VkImageAspectFlags aspectFlags;
	VkAccessFlags accessTypes;
	VkImageLayout startLayout;
	VkImageLayout finishLayout;
	VkImageUsageFlags usageFlags;
	std::optional<VImageResourceViewInfo> viewInfo;
};

struct VFramegraphBufferResource {
	VFramegraphNode* creator = nullptr;
	VBufferResourceHandle bufferResourceHandle;
	VFramegraphNodeBufferUsage usage;
};

struct VFramegraphImageResource {
	VFramegraphNode* creator = nullptr;
	VImageResourceHandle imageResourceHandle;
	VFramegraphNodeImageUsage usage;
};

struct VFramegraphNodeInfo {
	VFramegraphNode* node;

	std::unordered_map<std::string, VImageResourceViewInfo> resourceViewInfos;
	std::optional<VImageResourceViewInfo> swapchainResourceViewInfo;
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

	VkBufferUsageFlags bufferUsageFlags(const std::string& name) { return m_buffers[name].usage.usageFlags; }
	VkImageUsageFlags imageUsageFlags(const std::string& name) { return m_images[name].usage.usageFlags; }

	VkImageUsageFlags swapchainImageUsageFlags() const { return m_swapchainImageUsage.usageFlags; }

	void executeFrame(const AcquireResult& result, VkSemaphore signalSemaphore);

	void handleSwapchainResize(uint32_t width, uint32_t height);

  private:
	VGPUContext* m_gpuContext;
	VGPUResourceAllocator* m_resourceAllocator;

	std::vector<VFramegraphNodeInfo> m_nodes;

	std::vector<std::vector<VFramegraphBufferDependency>> m_nodeBufferDependencies;
	std::vector<std::vector<VFramegraphImageDependency>> m_nodeImageDependencies;

	std::unordered_map<std::string, VFramegraphBufferResource> m_buffers;
	std::unordered_map<std::string, VFramegraphImageResource> m_images;

	VFramegraphNodeImageUsage m_swapchainImageUsage = { .pipelineStages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT };
	std::unordered_map<size_t, VFramegraphImageDependency> m_nodeSwapchainImageDependencies;
	std::vector<std::unordered_map<VImageResourceViewInfo, VkImageView>> m_swapchainImageViews = { {} };
};

template <DerivesFrom<VFramegraphNode> T, typename... Args>
requires(ConstructibleWith<T, Args...>) inline VFramegraphNode* VFramegraphContext::appendNode(
	Args... constructorArgs) {
	m_nodes.push_back({ .node = new T(constructorArgs...) });
	return m_nodes.back().node;
}
