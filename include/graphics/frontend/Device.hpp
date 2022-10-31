/* VanadiumEngine, a Vulkan rendering toolkit
 * Copyright (C) 2022 Friedrich Vock
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#pragma once

#include <graphics/frontend/Pass.hpp>
#include <graphics/frontend/Pipeline.hpp>
#include <graphics/frontend/RefCounted.hpp>
#include <graphics/frontend/Resource.hpp>
#include <graphics/frontend/Transfer.hpp>
#include <string_view>

namespace vanadium::graphics::rhi {

	using AsyncBufferCopyHandle = SlotmapHandle;
	using AsyncImageUploadHandle = SlotmapHandle;

	class PerFrameBufferCopy {
	  public:
		const Buffer* dstBuffer() const { return m_dstBuffer; }

	  private:
		Buffer* m_dstBuffer;
	};

	struct BufferCopy {
		Buffer* src;
		Buffer* dst;
		size_t offset;
		size_t size;
	};

	struct ImageUpload {
		Buffer* src;
		Image* dst;
		size_t bufferOffset;
	};

	class Device {
	  public:
		virtual ~Device() = 0;

		virtual Buffer* createBuffer(size_t size, const MemoryAllocationType& type, bool isQueueShared) = 0;
		virtual Image* createImage(uint32_t width, uint32_t height, uint32_t depth, uint32_t mipCount,
								   uint32_t sampleCount, uint32_t arrayLayers, const MemoryAllocationType& type,
								   ImageUsage usage, bool isQueueShared) = 0;

		virtual Buffer* createBufferInBlock(MemoryBlock* block, size_t size, bool isQueueShared) = 0;
		virtual Image* createImageInBlock(MemoryBlock* block, uint32_t width, uint32_t height, uint32_t depth,
										  uint32_t mipCount, uint32_t sampleCount, uint32_t arrayLayers,
										  ImageUsage usage, bool isQueueShared) = 0;

		virtual MemoryBlock* createMemoryBlock(size_t size, const MemoryAllocationType& type,
											   MemoryBlockType blockType) = 0;

		virtual ImageView* createImageView(Image* image, Format format, const ComponentMapping& mapping,
										   const ImageSubresource& subresource) = 0;

		virtual Sampler* createSampler(Filter filter, Comparison comparison, SamplerOp op, AddressMode addressModeU,
									   AddressMode addressModeV, AddressMode addressModeW, float mipLODBias,
									   uint8_t maxAnisotropy, const float* borderColor, float minLOD, float maxLOD) = 0;

		virtual Pipeline* createPipeline(const std::string_view& name, const std::vector<Format>& renderTargetFormats,
										 Format depthStencilFormat) = 0;

		virtual void recordCommands(const std::vector<Pass>& passes) = 0;

		virtual void addBufferCopy(const BufferCopy& copy) = 0;
		virtual AsyncBufferCopyHandle createAsyncBufferCopy(const BufferCopy& copy) = 0;
		virtual PerFrameBufferCopy* createPerFrameBufferCopy(size_t bufferSize, const MemoryAllocationType& type) = 0;

		virtual void addImageUpload(const ImageUpload& upload) = 0;
		virtual AsyncImageUploadHandle createAsyncImageUpload(const ImageUpload& upload) = 0;

		virtual void submitAndPresent() = 0;
	};
} // namespace vanadium::graphics::rhi
