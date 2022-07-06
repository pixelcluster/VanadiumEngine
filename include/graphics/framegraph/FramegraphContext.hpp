#pragma once

#include <concepts>
#include <graphics/RenderContext.hpp>
#include <robin_hood.h>

#include <graphics/framegraph/QueueBarrierGenerator.hpp>

namespace vanadium::graphics {

	class FramegraphNode;

	struct FramegraphNodeBufferUsage {
		SimpleVector<NodeBufferSubresourceAccess> subresourceAccesses;
		bool writes;
		VkBufferUsageFlags usageFlags;
	};

	struct FramegraphNodeImageUsage {
		SimpleVector<NodeImageSubresourceAccess> subresourceAccesses;
		VkImageUsageFlags usageFlags;
		bool writes;
		SimpleVector<ImageResourceViewInfo> viewInfos;
	};

	struct FramegraphBufferCreationParameters {
		VkDeviceSize size;
		VkBufferCreateFlags flags;
	};

	struct FramegraphBufferResource {
		bool isImported = false;
		FramegraphBufferCreationParameters creationParameters;
		BufferResourceHandle resourceHandle = ~0U;

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
		bool useTargetImageExtent;
	};

	struct FramegraphImageResource {
		bool isImported = false;
		FramegraphImageCreationParameters creationParameters;
		VkImageUsageFlags usage;
		ImageResourceHandle resourceHandle = ~0U;
	};

	using FramegraphImageHandle = SlotmapHandle;

	struct FramegraphNodeInfo {
		FramegraphNode* node;

		robin_hood::unordered_map<FramegraphImageHandle, SimpleVector<ImageResourceViewInfo>> resourceViewInfos;
		SimpleVector<ImageResourceViewInfo> swapchainResourceViewInfos;
	};

	struct FramegraphNodeContext {
		uint32_t frameIndex;

		RenderTargetSurface* targetSurface;
		SimpleVector<VkImageView> targetImageViews;

		robin_hood::unordered_map<FramegraphImageHandle, SimpleVector<VkImageView>> resourceImageViews;
	};

	class FramegraphContext {
	  public:
		FramegraphContext() {}

		void create(const RenderContext& context);

		template <std::derived_from<FramegraphNode> T, typename... Args>
		requires(std::constructible_from<T, Args...>) T* insertNode(FramegraphNode* insertAfter,
																	Args... constructorArgs);

		// node must have been allocated with new, and you must call node->create() yourself
		template <std::derived_from<FramegraphNode> T> void insertExistingNode(FramegraphNode* insertAfter, T* node);

		void removeNode(FramegraphNode* node);

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

		const RenderContext& renderContext() { return m_context; }

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
		bool swapchainDirtyFlag() const { return m_swapchainDirtyFlag; }
		void clearSwapchainDirtyFlag() { m_swapchainDirtyFlag = false; }

		void destroy();

	  private:
		void createBuffer(FramegraphBufferHandle handle);
		void createImage(FramegraphImageHandle handle);

		// initResources handles initialization when usage etc. is known
		void initResources();
		void updateDependencyInfo();
		void updateBarriers();

		RenderContext m_context;

		QueueBarrierGenerator m_barrierGenerator;

		VkCommandPool m_frameCommandPools[frameInFlightCount];
		VkCommandBuffer m_frameCommandBuffers[frameInFlightCount];

		SimpleVector<FramegraphNodeInfo> m_nodes;

		Slotmap<FramegraphBufferResource> m_buffers;
		Slotmap<FramegraphImageResource> m_images;

		SimpleVector<FramegraphBufferHandle> m_transientBuffers;
		SimpleVector<FramegraphImageHandle> m_transientImages;

		bool m_resourceDirtyFlag = false;
		bool m_swapchainDirtyFlag = false;

		VkImageUsageFlags m_targetImageUsageFlags = 0;
	};

	template <std::derived_from<FramegraphNode> T, typename... Args>
	requires(std::constructible_from<T, Args...>) inline T* FramegraphContext::insertNode(FramegraphNode* insertAfter,
																						  Args... constructorArgs) {
		decltype(m_nodes)::iterator nodeIterator = m_nodes.begin();
		if (insertAfter) {
			nodeIterator = std::find_if(m_nodes.begin(), m_nodes.end(),
										[insertAfter](const auto& info) { return info.node == insertAfter; });
			if (nodeIterator == m_nodes.end()) {
				logError("Trying to insert after nonexistent node {}!", static_cast<void*>(insertAfter));
				return nullptr;
			}
			++nodeIterator;
		}
		size_t nodeIndex = static_cast<size_t>(nodeIterator - m_nodes.begin());
		if (nodeIterator == m_nodes.end()) {
			m_nodes.push_back({ .node = new T(constructorArgs...) });
		} else {
			m_nodes.insert(nodeIterator, { .node = new T(constructorArgs...) });
		}
		m_barrierGenerator.insertNodeBeforeIndex(nodeIndex);
		// FramegraphNode might not be defined at this point, but T will contain all of its methods
		reinterpret_cast<T*>(m_nodes[nodeIndex].node)->create(this);
		m_resourceDirtyFlag = true;
		return reinterpret_cast<T*>(m_nodes[nodeIndex].node);
	}

	template <std::derived_from<FramegraphNode> T>
	inline void FramegraphContext::insertExistingNode(FramegraphNode* insertAfter, T* node) {
		decltype(m_nodes)::iterator nodeIterator = m_nodes.begin();
		if (insertAfter) {
			nodeIterator = std::find_if(m_nodes.begin(), m_nodes.end(),
										[insertAfter](const auto& info) { return info.node == insertAfter; });
			if (nodeIterator == m_nodes.end()) {
				logError("Trying to insert after nonexistent node {}!", static_cast<void*>(insertAfter));
				return;
			}
			++nodeIterator;
		}
		size_t nodeIndex = static_cast<size_t>(nodeIterator - m_nodes.begin());
		if (nodeIterator == m_nodes.end()) {
			m_nodes.push_back({ .node = node });
		} else {
			m_nodes.insert(nodeIterator, { .node = node });
		}
		m_barrierGenerator.insertNodeBeforeIndex(nodeIndex);
		m_resourceDirtyFlag = true;
	}
} // namespace vanadium::graphics