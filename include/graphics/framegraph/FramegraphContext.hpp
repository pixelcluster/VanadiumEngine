#pragma once

#include <concepts>
#include <graphics/RenderContext.hpp>
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

		void create(const RenderContext& context);

		template <std::derived_from<FramegraphNode> T, typename... Args>
		requires(std::constructible_from<T, Args...>) T* appendNode(Args... constructorArgs);

		// node must have been allocated with new, and you must call node->create() yourself
		template <std::derived_from<FramegraphNode> T> void appendExistingNode(T* node);

		//setupResources and initResources should be called together
		//setupResources determines usage information and barriers
		//At the time setupResources is called, swapchain formats are potentially unknown!
		void setupResources();
		//initResources handles initialization when usage etc. is known
		void initResources();

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

		void destroy();

	  private:
		void createBuffer(FramegraphBufferHandle handle);
		void createImage(FramegraphImageHandle handle);

		void updateDependencyInfo();
		void updateBarriers();

		RenderContext m_context;

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

	template <std::derived_from<FramegraphNode> T> inline void FramegraphContext::appendExistingNode(T* node) {
		m_nodes.push_back({ .node = node });
	}
} // namespace vanadium::graphics