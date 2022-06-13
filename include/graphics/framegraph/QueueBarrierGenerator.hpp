#pragma once

#define VK_NO_PROTOTYPES
#include <optional>
#include <robin_hood.h>
#include <vector>
#include <vulkan/vulkan.h>
#include <helper/Slotmap.hpp>

namespace vanadium::graphics {
	class FramegraphContext;

	struct NodeImageSubresourceAccess {
		VkPipelineStageFlags accessingPipelineStages;
		VkAccessFlags access;
		VkImageSubresourceRange subresourceRange;
		VkImageLayout startLayout;
		VkImageLayout finishLayout;
		bool writes;
	};

	struct NodeImageAccess {
		std::vector<NodeImageSubresourceAccess> subresourceAccesses;
		bool preserveAcrossFrames;
		VkImageLayout initialLayout;
		SlotmapHandle image;
	};

	struct ImageSubresourceAccess {
		VkPipelineStageFlags accessingPipelineStages;
		VkAccessFlags access;
		VkImageSubresourceRange subresourceRange;
		VkImageLayout startLayout;
		VkImageLayout finishLayout;
		size_t nodeIndex;
		bool operator<(const ImageSubresourceAccess& other) const { return nodeIndex < other.nodeIndex; }
	};

	struct ImageAccessInfo {
		std::vector<ImageSubresourceAccess> reads;
		std::vector<ImageSubresourceAccess> modifications;
		bool preserveAcrossFrames = false;
		VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		bool isNew;
	};

	struct ImageFramegraphBarrier {
		size_t dstNodeIndex;

		VkPipelineStageFlags srcPipelineStageFlags;
		VkPipelineStageFlags dstPipelineStageFlags;
		VkAccessFlags srcAccessFlags;
		VkAccessFlags dstAccessFlags;
		VkImageSubresourceRange subresourceRange;
		VkImageLayout beforeLayout;
		VkImageLayout afterLayout;
		// If no value, the barrier refers to the target surface
		std::optional<SlotmapHandle> image;
	};

	struct NodeBufferSubresourceAccess {
		VkPipelineStageFlags accessingPipelineStages;
		VkAccessFlags access;
		VkDeviceSize offset;
		VkDeviceSize size;
		bool writes;
	};

	struct NodeBufferAccess {
		std::vector<NodeBufferSubresourceAccess> subresourceAccesses;
		SlotmapHandle buffer;
	};

	struct BufferSubresourceAccess {
		VkPipelineStageFlags accessingPipelineStages;
		VkAccessFlags access;
		VkDeviceSize offset;
		VkDeviceSize size;
		size_t nodeIndex;
		bool operator<(const BufferSubresourceAccess& other) const { return nodeIndex < other.nodeIndex; }
	};

	struct BufferAccessInfo {
		std::vector<BufferSubresourceAccess> reads;
		std::vector<BufferSubresourceAccess> modifications;
	};

	struct BufferFramegraphBarrier {
		size_t dstNodeIndex;

		VkPipelineStageFlags srcPipelineStageFlags;
		VkPipelineStageFlags dstPipelineStageFlags;
		VkAccessFlags srcAccessFlags;
		VkAccessFlags dstAccessFlags;
		VkDeviceSize offset;
		VkDeviceSize size;
		SlotmapHandle buffer;
	};

	struct NodeBarrierInfo {
		std::vector<ImageFramegraphBarrier> imageBarriers;
		std::vector<BufferFramegraphBarrier> bufferBarriers;

		std::vector<VkImageMemoryBarrier> vulkanImageBarriers;
		std::vector<VkBufferMemoryBarrier> vulkanBufferBarriers;
		VkPipelineStageFlags srcStages;
		VkPipelineStageFlags dstStages;
	};

	struct BufferAccessMatch {
		size_t matchingNodeIndex;
		size_t accessIndex;
		bool isPartial;
		size_t offset;
		size_t size;
	};

	struct ImageAccessMatch {
		size_t matchingNodeIndex;
		size_t accessIndex;
		bool isPartial;
		VkImageSubresourceRange range;
	};

	using BufferHandleRetriever = VkBuffer (FramegraphContext::*)(SlotmapHandle handle);
	using ImageHandleRetriever = VkImage (FramegraphContext::*)(SlotmapHandle handle);

	class QueueBarrierGenerator {
	  public:
		void create(size_t nodeCount);

		void addNodeBufferAccess(size_t nodeIndex, const NodeBufferAccess& bufferAccess);
		void addNodeImageAccess(size_t nodeIndex, const NodeImageAccess& imageAccess);
		void addNodeTargetImageAccess(size_t nodeIndex, const NodeImageAccess& imageAccess);

		void insertNodeBeforeIndex(size_t nodeIndex);
		void removeNodeIndex(size_t nodeIndex);

		std::vector<SlotmapHandle> unusedBuffers() const;
		std::vector<SlotmapHandle> unusedImages() const;

		void generateDependencyInfo();

		void generateBarrierInfo(BufferHandleRetriever bufferHandleRetriever,
								 ImageHandleRetriever imageHandleRetriever, FramegraphContext* context,
								 VkImage currentTargetImageHandle);

		size_t bufferBarrierCount(size_t nodeIndex) const;
		const std::vector<VkBufferMemoryBarrier>& bufferBarriers(size_t nodeIndex) const;
		size_t imageBarrierCount(size_t nodeIndex) const;
		const std::vector<VkImageMemoryBarrier>& imageBarriers(size_t nodeIndex) const;
		VkPipelineStageFlags srcStages(size_t nodeIndex) const;
		VkPipelineStageFlags dstStages(size_t nodeIndex) const;

		size_t frameStartBarrierCount() const { return m_frameStartImageBarriers.size(); }
		const std::vector<VkImageMemoryBarrier>& frameStartBarriers() const { return m_vulkanFrameStartImageBarriers; }

		VkImageLayout lastTargetImageLayout() const;
		VkImageSubresourceRange lastTargetAccessRange() const;

	  private:
		void emitBarriersForRead(size_t nodeIndex, SlotmapHandle buffer, const BufferAccessInfo& info,
								 const BufferSubresourceAccess& read);
		void emitBarriersForRead(size_t nodeIndex, std::optional<SlotmapHandle> image,
								 const ImageAccessInfo& info, const ImageSubresourceAccess& read);

		std::optional<BufferAccessMatch> findLastModification(const std::vector<BufferSubresourceAccess>& modifications,
															  const BufferSubresourceAccess& read);
		std::optional<ImageAccessMatch> findLastModification(const std::vector<ImageSubresourceAccess>& modifications,
															 const ImageSubresourceAccess& read);

		void emitBarrier(SlotmapHandle buffer, const BufferAccessMatch& match,
						 const BufferSubresourceAccess& write, const BufferSubresourceAccess& read);
		void emitBarrier(std::optional<SlotmapHandle> image, const ImageAccessMatch& match,
						 const ImageSubresourceAccess& write, const ImageSubresourceAccess& read);

		robin_hood::unordered_map<SlotmapHandle, BufferAccessInfo> m_bufferAccessInfos;

		robin_hood::unordered_map<SlotmapHandle, ImageAccessInfo> m_imageAccessInfos;
		ImageAccessInfo m_targetAccessInfo;

		std::vector<NodeBarrierInfo> m_nodeBarrierInfos;

		std::vector<ImageFramegraphBarrier> m_frameStartImageBarriers;
		std::vector<VkImageMemoryBarrier> m_vulkanFrameStartImageBarriers;
	};

} // namespace vanadium::graphics
