#pragma once

#include <modules/gpu/framegraph/VFramegraphNode.hpp>
#include <glm/glm.hpp>

struct CameraSceneData {
	glm::mat4 viewProjection;
};

class PlanetRenderNode : public VFramegraphNode {
  public:
	PlanetRenderNode(VBufferResourceHandle vertexDataBuffer, uint32_t indexCount);

	void setupResources(VFramegraphContext* context);

	void initResources(VFramegraphContext* context);

	void recordCommands(VFramegraphContext* context, VkCommandBuffer targetCommandBuffer,
								const VFramegraphNodeContext& nodeContext);

	void handleWindowResize(VFramegraphContext* context, uint32_t width, uint32_t height);

	void destroyResources(VFramegraphContext* context);
  private:
	VBufferResourceHandle m_vertexData;
	VGPUTransferHandle m_sceneDataTransfer;

	uint32_t m_indexCount;

	CameraSceneData sceneData;

	VkPipeline m_graphicsPipeline;
	VkPipelineLayout m_pipelineLayout;
	VkDescriptorSetLayout m_setLayout;
	VkDescriptorSet m_uboSet;
	VkRenderPass m_renderPass;

	uint32_t m_width, m_height;
	std::vector<VkFramebuffer> m_framebuffers;
};