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

#include <graphics/framegraph/FramegraphNode.hpp>
#include <graphics/util/GPUTransferManager.hpp>
#include <VertexBufferUpdater.hpp>

class HelloTriangleNode : public vanadium::graphics::FramegraphNode {
  public:
	HelloTriangleNode(VertexBufferUpdater* bufferUpdater);

	void create(vanadium::graphics::FramegraphContext* context) override;

	void recordCommands(vanadium::graphics::FramegraphContext* context, VkCommandBuffer targetCommandBuffer,
						const vanadium::graphics::FramegraphNodeContext& nodeContext) override;

	void recreateSwapchainResources(vanadium::graphics::FramegraphContext* context, uint32_t width, uint32_t height) override;

	void destroy(vanadium::graphics::FramegraphContext* context) override;

  private:
	VertexBufferUpdater* m_bufferUpdater;
	vanadium::graphics::RenderPassSignature m_passSignature;
	VkRenderPass m_renderPass;
	uint32_t m_pipelineID;

	vanadium::graphics::ImageResourceViewInfo m_swapchainViewInfo;

	uint32_t m_width, m_height;
	std::vector<VkFramebuffer> m_framebuffers;
};
