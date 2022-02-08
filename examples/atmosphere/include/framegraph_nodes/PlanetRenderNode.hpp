#pragma once

#include <modules/gpu/framegraph/VFramegraphNode.hpp>
#include <glm/glm.hpp>

class PlanetRenderNode : public VFramegraphNode {
  public:
	PlanetRenderNode(VBufferResourceHandle vertexDataBuffer, VBufferResourceHandle indexDataBuffer,
					 VBufferResourceHandle sceneDataBuffer, VImageResourceHandle texHandle, uint32_t indexCount);

	void setupResources(VFramegraphContext* context);

	void initResources(VFramegraphContext* context);

	void recordCommands(VFramegraphContext* context, VkCommandBuffer targetCommandBuffer,
								const VFramegraphNodeContext& nodeContext);

	void handleWindowResize(VFramegraphContext* context, uint32_t width, uint32_t height);

	void destroyResources(VFramegraphContext* context);
  private:
	VBufferResourceHandle m_vertexData;
	VBufferResourceHandle m_indexData;
	VBufferResourceHandle m_uboHandle;
	VImageResourceHandle m_texHandle;

	uint32_t m_indexCount;

	VkSampler m_texSampler;

	VkDescriptorSetLayout m_setLayout;
	VkDescriptorSet m_uboSet;
	VkDescriptorSetLayout m_texSetLayout;
	VkDescriptorSet m_texSet;

	VkPipeline m_graphicsPipeline;
	VkPipelineLayout m_pipelineLayout;
	VkRenderPass m_renderPass;

	uint32_t m_width, m_height;
	std::vector<VkFramebuffer> m_framebuffers;
};