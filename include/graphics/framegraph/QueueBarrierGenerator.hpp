#pragma once

#define VK_NO_PROTOTYPES
#include <optional>
#include <robin_hood.h>
#include <util/Vector.hpp>
#include <vulkan/vulkan.h>
#include <util/Slotmap.hpp>

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
		SimpleVector<NodeImageSubresourceAccess> subresourceAccesses;
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
		SimpleVector<ImageSubresourceAccess> reads;
		SimpleVector<ImageSubresourceAccess> modifications;
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
		SimpleVector<NodeBufferSubresourceAccess> subresourceAccesses;
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
		SimpleVector<BufferSubresourceAccess> reads;
		SimpleVector<BufferSubresourceAccess> modifications;
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
		SimpleVector<ImageFramegraphBarrier> imageBarriers;
		SimpleVector<BufferFramegraphBarrier> bufferBarriers;

		SimpleVector<VkImageMemoryBarrier> vulkanImageBarriers;
		SimpleVector<VkBufferMemoryBarrier> vulkanBufferBarriers;
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

		SimpleVector<SlotmapHandle> unusedBuffers() const;
		SimpleVector<SlotmapHandle> unusedImages() const;

		void generateDependencyInfo();

		void generateBarrierInfo(BufferHandleRetriever bufferHandleRetriever,
								 ImageHandleRetriever imageHandleRetriever, FramegraphContext* context,
								 VkImage currentTargetImageHandle);

		size_t bufferBarrierCount(size_t nodeIndex) const;
		const SimpleVector<VkBufferMemoryBarrier>& bufferBarriers(size_t nodeIndex) const;
		size_t imageBarrierCount(size_t nodeIndex) const;
		const SimpleVector<VkImageMemoryBarrier>& imageBarriers(size_t nodeIndex) const;
		VkPipelineStageFlags srcStages(size_t nodeIndex) const;
		VkPipelineStageFlags dstStages(size_t nodeIndex) const;

		size_t frameStartBarrierCount() const { return m_frameStartImageBarriers.size(); }
		const SimpleVector<VkImageMemoryBarrier>& frameStartBarriers() const { return m_vulkanFrameStartImageBarriers; }

		VkImageLayout lastTargetImageLayout() const;
		VkImageSubresourceRange lastTargetAccessRange() const;

	  private:
		void emitBarriersForRead(size_t nodeIndex, SlotmapHandle buffer, const BufferAccessInfo& info,
								 const BufferSubresourceAccess& read);
		void emitBarriersForRead(size_t nodeIndex, std::optional<SlotmapHandle> image,
								 const ImageAccessInfo& info, const ImageSubresourceAccess& read);

		std::optional<BufferAccessMatch> findLastModification(const SimpleVector<BufferSubresourceAccess>& modifications,
															  const BufferSubresourceAccess& read);
		std::optional<ImageAccessMatch> findLastModification(const SimpleVector<ImageSubresourceAccess>& modifications,
															 const ImageSubresourceAccess& read);

		void emitBarrier(SlotmapHandle buffer, const BufferAccessMatch& match,
						 const BufferSubresourceAccess& write, const BufferSubresourceAccess& read);
		void emitBarrier(std::optional<SlotmapHandle> image, const ImageAccessMatch& match,
						 const ImageSubresourceAccess& write, const ImageSubresourceAccess& read);

		robin_hood::unordered_map<SlotmapHandle, BufferAccessInfo> m_bufferAccessInfos;

		robin_hood::unordered_map<SlotmapHandle, ImageAccessInfo> m_imageAccessInfos;
		ImageAccessInfo m_targetAccessInfo;

		SimpleVector<NodeBarrierInfo> m_nodeBarrierInfos;

		SimpleVector<ImageFramegraphBarrier> m_frameStartImageBarriers;
		SimpleVector<VkImageMemoryBarrier> m_vulkanFrameStartImageBarriers;
	};

} // namespace vanadium::graphics
