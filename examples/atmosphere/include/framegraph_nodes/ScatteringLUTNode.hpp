#pragma once

#include <modules/gpu/framegraph/VFramegraphNode.hpp>
#include <glm/glm.hpp>

struct TransmittanceComputeData {
	glm::ivec4 lutSize;
	glm::vec4 betaExtinctionZeroMie;
	glm::vec4 betaExtinctionZeroRayleigh;
	float heightScaleRayleigh;
	float heightScaleMie;
	float maxHeight;
	float groundRadius;
};

class ScatteringLUTNode : public VFramegraphNode {
  public:
	ScatteringLUTNode() { m_name = "Scattering LUT \"pre\"computation"; }

	virtual void create(VFramegraphContext* context);

	virtual void setupResources(VFramegraphContext* context) {}

	virtual void initResources(VFramegraphContext* context);

	virtual void recordCommands(VFramegraphContext* context, VkCommandBuffer targetCommandBuffer,
								const VFramegraphNodeContext& nodeContext);

	virtual void handleWindowResize(VFramegraphContext* context, uint32_t width, uint32_t height) {}

	virtual void destroy(VFramegraphContext* context);

	VFramegraphImageHandle transmittanceLUTHandle() { return m_transmittanceLUTHandle; }
	VFramegraphImageHandle skyViewLUTHandle() { return m_skyViewLUTHandle; }
	VFramegraphImageHandle aerialPerspectiveLUTHandle() { return m_aerialPerspectiveLUTHandle; }
	VFramegraphImageHandle multiscatterLUTHandle() { return m_multiscatterLUTHandle; }

  private:
	VFramegraphImageHandle m_transmittanceLUTHandle;
	VFramegraphImageHandle m_skyViewLUTHandle;
	VFramegraphImageHandle m_aerialPerspectiveLUTHandle;
	VFramegraphImageHandle m_multiscatterLUTHandle;

	VkPipeline m_transmittanceComputationPipeline;

	VkDescriptorSetLayout m_pipelineOutputLayout;
	VkPipelineLayout m_transmittanceComputationPipelineLayout;
	VkPipelineLayout m_skyViewComputationPipelineLayout;
	VkPipelineLayout m_aerialPerspectiveComputationPipelineLayout;
	VkPipelineLayout m_multiscatterComputationPipelineLayout;

	VkDescriptorSet m_transmittanceComputationOutputSet;
	VkDescriptorSet m_skyViewComputationOutputSet;
	VkDescriptorSet m_aerialPerspectiveComputationOutputSet;
	VkDescriptorSet m_multiscatterComputationOutputSet;

	VkDescriptorSetLayout m_lutSetLayout;
	VkDescriptorSet m_lutSet;
};