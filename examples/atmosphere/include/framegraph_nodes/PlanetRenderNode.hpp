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

#include <PlanetObject.hpp>
#include <glm/glm.hpp>
#include <graphics/framegraph/FramegraphNode.hpp>

class PlanetRenderNode : public vanadium::graphics::FramegraphNode {
  public:
	PlanetRenderNode();

	void create(vanadium::graphics::FramegraphContext* context);

	void setupObjects(vanadium::graphics::BufferResourceHandle vertexDataBuffer, vanadium::graphics::BufferResourceHandle indexDataBuffer,
					  VkDescriptorSetLayout sceneDataLayout, VkDescriptorSet sceneDataSet,
					  VkDescriptorSetLayout texSetLayout, VkDescriptorSet texSet, uint32_t indexCount);

	void recordCommands(vanadium::graphics::FramegraphContext* context, VkCommandBuffer targetCommandBuffer,
						const vanadium::graphics::FramegraphNodeContext& nodeContext);

	void recreateSwapchainResources(vanadium::graphics::FramegraphContext* context, uint32_t width, uint32_t height);

	void destroy(vanadium::graphics::FramegraphContext* context);

  private:
	vanadium::graphics::BufferResourceHandle m_vertexData;
	vanadium::graphics::BufferResourceHandle m_indexData;

	uint32_t m_indexCount;

	VkSampler m_texSampler;

	VkDescriptorSetLayout m_uboSetLayout;
	VkDescriptorSet m_uboSet;
	VkDescriptorSetLayout m_texSetLayout;
	VkDescriptorSet m_texSet;

	uint32_t m_pipelineID;
	vanadium::graphics::RenderPassSignature m_passSignature;
	VkRenderPass m_renderPass;

	uint32_t m_width, m_height;
	std::vector<VkFramebuffer> m_framebuffers;
};
