#pragma once

#include <modules/gpu/VGPUResourceAllocator.hpp>
#include <string_view>

class VFramegraphNode;

struct VFramegraphBufferDependency {
	std::string resourceName;
	VkPipelineStageFlags srcStages;
	VkPipelineStageFlags dstStages;
	VkAccessFlags srcAccesses;
	VkAccessFlags dstAccesses;
};

struct VFramegraphImageDependency {
	std::string resourceName;
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

struct VFramegraphImageResourceView {
	VImageResourceViewInfo info;
	VkImageView handle;
};

struct VFramegraphNodeInfo {
	VFramegraphNode* node;
	std::unordered_map<std::string, VFramegraphImageResourceView> resourceViewInfos;
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

	void declareImportedBuffer(const std::string_view& name, VBufferResourceHandle handle,
							   const VFramegraphNodeBufferUsage& usage);
	void declareImportedImage(const std::string_view& name, VImageResourceHandle handle,
							  const VFramegraphNodeImageUsage& usage);

	// These functions depend on the fact that all references are inserted in execution order!
	void declareReferencedBuffer(VFramegraphNode* user, const std::string_view& name,
								 const VFramegraphNodeBufferUsage& usage);
	void declareReferencedImage(VFramegraphNode* user, const std::string_view& name,
								const VFramegraphNodeImageUsage& usage);

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

	VkBufferUsageFlags bufferUsageFlags(const std::string& name) { return m_buffers[name].usage.usageFlags; }
	VkImageUsageFlags imageUsageFlags(const std::string& name) { return m_images[name].usage.usageFlags; }

	void executeFrame(const AcquireResult& result, VkSemaphore signalSemaphore);

  private:
	VGPUContext* m_gpuContext;
	VGPUResourceAllocator* m_resourceAllocator;

	std::vector<VFramegraphNodeInfo> m_nodes;

	std::vector<std::vector<VFramegraphBufferDependency>> m_nodeBufferDependencies;
	std::vector<std::vector<VFramegraphImageDependency>> m_nodeImageDependencies;

	std::unordered_map<std::string, VFramegraphBufferResource> m_buffers;
	std::unordered_map<std::string, VFramegraphImageResource> m_images;
};

template <DerivesFrom<VFramegraphNode> T, typename... Args>
requires(ConstructibleWith<T, Args...>) inline VFramegraphNode* VFramegraphContext::appendNode(
	Args... constructorArgs) {
	m_nodes.push_back({ .node = new T(constructorArgs...) });
	return m_nodes.back().node;
}
