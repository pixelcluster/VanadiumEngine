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
};

struct VFramegraphNodeImageUsage {
	VkPipelineStageFlags pipelineStages;
	VkAccessFlags accessTypes;
	VkImageLayout startLayout;
	VkImageLayout finishLayout;
};

struct VFramegraphBufferResource {
	VFramegraphNode* creator = nullptr;
	VBufferResourceHandle bufferResourceHandle;
};

struct VFramegraphImageResource {
	VFramegraphNode* creator = nullptr;
	VImageResourceHandle imageResourceHandle;
};

class VFramegraphContext {
  public:
	VFramegraphContext() {}

	void create(VGPUContext* context, VGPUResourceAllocator* resourceAllocator);

	template<DerivesFrom<VFramegraphNode> T, typename... Args> requires(ConstructibleWith<T, Args...>) 
	VFramegraphNode* appendNode(Args... constructorArgs);

	void declareCreatedBuffer(VFramegraphNode* creator, const std::string_view& name, const VFramegraphNodeBufferUsage& usage);
	void declareCreatedImage(VFramegraphNode* creator, const std::string_view& name,
							 const VFramegraphNodeImageUsage& usage);

	void declareImportedBuffer(const std::string_view& name, VBufferResourceHandle handle,
							   const VFramegraphNodeBufferUsage& usage);
	void declareImportedImage(const std::string_view& name, VImageResourceHandle handle,
							  const VFramegraphNodeImageUsage& usage);

	//These functions depend on the fact that all references are inserted in execution order!
	void declareReferencedBuffer(VFramegraphNode* user, const std::string_view& name,
								 const VFramegraphNodeBufferUsage& usage);
	void declareReferencedImage(VFramegraphNode* user, const std::string_view& name, const VFramegraphNodeImageUsage& usage);

	VGPUContext* gpuContext() { return m_gpuContext; }
	VGPUResourceAllocator* resourceAllocator() { return m_resourceAllocator; }

	VkImage nativeImageHandle(const std::string& name) {
		return m_resourceAllocator->nativeImageHandle(m_images[name].imageResourceHandle);
	}

  private:
	VGPUContext* m_gpuContext;
	VGPUResourceAllocator* m_resourceAllocator;

	std::vector<VFramegraphNode*> m_nodes;

	std::vector<std::vector<VFramegraphBufferDependency>> m_nodeBufferDependencies;
	std::vector<std::vector<VFramegraphImageDependency>> m_nodeImageDependencies;

	std::unordered_map<std::string, VFramegraphBufferResource> m_buffers;
	std::unordered_map<std::string, VFramegraphImageResource> m_images;

	std::unordered_map<std::string, VFramegraphNodeBufferUsage> m_bufferUsages;
	std::unordered_map<std::string, VFramegraphNodeImageUsage> m_imageUsages;
};

template <DerivesFrom<VFramegraphNode> T, typename... Args>
requires(ConstructibleWith<T, Args...>) inline VFramegraphNode* VFramegraphContext::appendNode(Args... constructorArgs) {
	m_nodes.push_back(new T(constructorArgs...));
	return m_nodes.back();
}
