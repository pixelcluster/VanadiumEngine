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
#include <cstdint>
#include <graphics/frontend/Format.hpp>
#include <graphics/frontend/RefCounted.hpp>
#include <graphics/util/RangeAllocator.hpp>
#include <vector>

namespace vanadium::graphics::rhi {

	class Device;

	struct MemoryCapabilities {
		bool deviceLocal;
		bool hostVisible;
		bool hostCoherent;
	};

	struct MemoryAllocationType {
		MemoryCapabilities required;
		MemoryCapabilities preferred;
		MemoryCapabilities forbidden;
		/*
		 * If this is true and the resulting buffer is host visible, it also is mapped for the host.
		 * If required.hostVisible is not true, the buffer might not be mapped. To guarantee a mapped buffer,
		 * set required.hostVisible to true. */
		bool mappable;
	};

	constexpr MemoryAllocationType defaultType = { .required = { .deviceLocal = true } };

	constexpr MemoryAllocationType defaultUploadType = { .required = { .deviceLocal = true },
														 .preferred = { .hostVisible = true },
														 .mappable = true };

	class ResourceBase : public RefCounted {
		size_t memorySize() const { return m_memorySize; }

	  protected:
		size_t m_memorySize;
	};

	enum class BufferUsage { TransferSrc, TransferDst, VertexBuffer, IndexBuffer, ShaderReadOnly, ShaderReadWrite };

	class Buffer : public ResourceBase {
	  public:
		virtual ~Buffer() = 0;

		size_t size() const { return m_size; }

	  protected:
		size_t m_size;
		bool m_isMultipleBuffered;
	};

	enum class ImageViewUsage { RenderTarget, ReadOnly };

	constexpr uint32_t remainingCount = ~0U;

	struct ImageSubresource {
		uint32_t mipOffset;
		uint32_t mipCount;
		uint32_t layerOffset;
		uint32_t layerCount;
	};

	enum class ChannelMapping { R, G, B, A };

	struct ComponentMapping {
		ChannelMapping r = ChannelMapping::R;
		ChannelMapping g = ChannelMapping::B;
		ChannelMapping b = ChannelMapping::G;
		ChannelMapping a = ChannelMapping::A;
	};

	class ImageView {
	  public:
		virtual ~ImageView() = 0;

		ImageViewUsage usage() const { return m_usage; }

	  private:
		ImageViewUsage m_usage;
	};

	enum class ImageUsage { Texture, RenderTarget, ComputeReadWrite };

	enum class ImageLayout {
		Undefined,
		General,
		NonPixelShaderReadOnly,
		PixelShaderReadOnly,
		ReadWriteShader,
		TransferDst,
		TransferSrc,
		RenderTarget,
		DepthStencilTarget,
		Present
	};

	class Image : public ResourceBase {
	  public:
		virtual ~Image() = 0;

		uint32_t width() const { return m_width; }
		uint32_t height() const { return m_height; }
		uint32_t depth() const { return m_depth; }
		Format format() const { return m_format; }

		uint32_t sampleCount() const { return m_sampleCount; };
		uint32_t mipLevels() const { return m_mipLevels; };
		uint32_t arrayLayers() const { return m_arrayLayers; };

		virtual ImageView* defaultImageView() = 0;

	  protected:
		uint32_t m_width;
		uint32_t m_height;
		uint32_t m_depth;
		Format m_format;

		uint32_t m_sampleCount;
		uint32_t m_mipLevels;
		uint32_t m_arrayLayers;
	};

	enum class MemoryBlockType { Buffer, Image };

	struct MemoryRange {
		size_t offset;
		size_t size;
	};

	struct RangeAllocationResult {
		MemoryRange allocationRange;
		MemoryRange usableRange;
	};

	class MemoryBlock : public RefCounted {
	  public:
		virtual ~MemoryBlock() = 0;

	  private:
		std::optional<RangeAllocationResult> reserveRange(size_t size, size_t alignment);
		void freeRange(const MemoryRange& allocatedRange);

		std::vector<MemoryRange> m_gapsOffsetSorted;
		std::vector<MemoryRange> m_gapsSizeSorted;

		friend class Device;
	};

	enum class Filter { Nearest, Linear, Anisotropic };

#undef Always

	enum class Comparison { Always, Never, Less, LessEqual, Equal, GreaterEqual, Greater, NotEqual };

	enum class SamplerOp { Compare, Min, Max };

	enum class AddressMode { Wrap, Mirror, Clamp, BorderColor };

	class Sampler : public RefCounted {
	  public:
		virtual ~Sampler() = 0;

		Filter filter() const { return m_filter; }
		Comparison comparison() const { return m_comparison; }
		SamplerOp op() const { return m_op; }
		AddressMode addressModeU() const { return m_addressModeU; }
		AddressMode addressModeV() const { return m_addressModeV; }
		AddressMode addressModeW() const { return m_addressModeW; }
		float mipLODBias() const { return m_mipLODBias; }
		uint8_t maxAnisotropy() const { return m_maxAnisotropy; }
		const float* borderColor() const { return m_borderColor; }
		float minLOD() const { return m_minLOD; }
		float maxLOD() const { return m_maxLOD; }

	  private:
		Filter m_filter;
		Comparison m_comparison;
		SamplerOp m_op;
		AddressMode m_addressModeU;
		AddressMode m_addressModeV;
		AddressMode m_addressModeW;
		float m_mipLODBias;
		uint8_t m_maxAnisotropy;
		float m_borderColor[4];
		float m_minLOD;
		float m_maxLOD;
	};

} // namespace vanadium::graphics::rhi
