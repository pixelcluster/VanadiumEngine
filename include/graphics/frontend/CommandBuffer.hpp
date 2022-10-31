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

#include <graphics/frontend/Pipeline.hpp>
#include <graphics/frontend/Resource.hpp>
#include <graphics/helper/EnumClassFlags.hpp>

namespace vanadium::graphics::rhi {

#undef None

	enum class StageScope {
		BeforeVertexShader = 0x1,
		VertexShader = 0x2,
		FragmentShader = 0x4,
		EarlyFragmentTests = 0x8,
		LateFragmentTests = 0x10,
		AttachmentOutput = 0x20,
		Transfer = 0x40,
		All = 0x7F,
		None = 0x0
	};

	DEFINE_FLAGS(StageScope)

	enum class AccessScope {
		IndirectRead = 0x1,
		VertexAttributeRead = 0x2,
		IndexRead = 0x4,
		ReadOnlyResourceRead = 0x8,
		ShaderRead = 0x10,
		ShaderWrite = 0x20,
		RenderTargetRead = 0x40,
		RenderTargetWrite = 0x80,
		DepthStencilRead = 0x100,
		DepthStencilWrite = 0x200,
		TransferRead = 0x400,
		TransferWrite = 0x800,
		HostRead = 0x1000,
		HostWrite = 0x2000,
		All = 0x3FFF,
		None = 0
	};

	DEFINE_FLAGS(AccessScope)

	struct BufferBarrier {
		StageScope src_stage;
		AccessScope src_access;
		StageScope dst_stage;
		AccessScope dst_access;

		Buffer* buffer;
	};

	struct ImageBarrier {
		StageScope src_stage;
		AccessScope src_access;
		StageScope dst_stage;
		AccessScope dst_access;

		ImageLayout prevLayout;
		ImageLayout nextLayout;

		Image* image;
	};

	enum class PipelineBindPoint { Graphics, Compute };

	class CommandBuffer {
	  public:
		virtual ~CommandBuffer() = 0;

		virtual void recordBarrier(const std::vector<BufferBarrier>& bufferBarriers,
								   const std::vector<ImageBarrier>& imageBarriers) = 0;

		virtual void recordSetViewport(const Viewport& newViewport) = 0;
		virtual void recordSetScissor(const ScissorRect& newScissor) = 0;
		virtual void recordSetVertexBuffers(const std::vector<Buffer>& vertexBuffers) = 0;
		virtual void recordSetIndexBuffer(const Buffer& indexBuffer) = 0;

		virtual void recordUpdateDescriptors(PipelineBindPoint bindPoint, ) = 0;
		virtual void recordBindDescriptorSet(PipelineBindPoint bindPoint, uint32_t setIndex, const DescriptorSet& set) = 0;
		virtual void recordBindPipeline(PipelineBindPoint bindPoint, const Pipeline& pipeline) = 0;

		virtual void recordDispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) = 0;
		virtual void recordDraw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex,
								uint32_t firstInstance) = 0;
		virtual void recordIndexedDraw(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex,
									   int32_t vertexOffset, uint32_t firstInstance) = 0;

	  private:
		virtual void beginRecording() = 0;
		virtual void recordPassBegin() = 0;
		virtual void recordPassEnd() = 0;
	};
} // namespace vanadium::graphics::rhi
