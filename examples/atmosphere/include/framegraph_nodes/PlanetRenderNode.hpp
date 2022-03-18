#pragma once

#include <PlanetObject.hpp>
#include <glm/glm.hpp>
#include <graphics/framegraph/FramegraphNode.hpp>

class PlanetRenderNode : public FramegraphNode {
  public:
	PlanetRenderNode();

	void create(FramegraphContext* context);

	void setupObjects(BufferResourceHandle vertexDataBuffer, BufferResourceHandle indexDataBuffer,
					  VkDescriptorSetLayout sceneDataLayout, VkDescriptorSet sceneDataSet,
					  VkDescriptorSetLayout texSetLayout, VkDescriptorSet texSet, uint32_t indexCount);

	void setupResources(FramegraphContext* context);

	void initResources(FramegraphContext* context);

	void recordCommands(FramegraphContext* context, VkCommandBuffer targetCommandBuffer,
						const FramegraphNodeContext& nodeContext);

	void handleWindowResize(FramegraphContext* context, uint32_t width, uint32_t height);

	void destroy(FramegraphContext* context);

  private:
	BufferResourceHandle m_vertexData;
	BufferResourceHandle m_indexData;

	uint32_t m_indexCount;

	VkSampler m_texSampler;

	VkDescriptorSetLayout m_uboSetLayout;
	VkDescriptorSet m_uboSet;
	VkDescriptorSetLayout m_texSetLayout;
	VkDescriptorSet m_texSet;

	VkPipeline m_graphicsPipeline;
	VkPipelineLayout m_pipelineLayout;
	VkRenderPass m_renderPass;

	uint32_t m_width, m_height;
	std::vector<VkFramebuffer> m_framebuffers;
};