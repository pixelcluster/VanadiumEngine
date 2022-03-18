#pragma once

#include <graphics/framegraph/FramegraphNode.hpp>
#include <glm/glm.hpp>

struct TransmittanceComputeData {
	glm::ivec4 lutSize;
	glm::vec4 betaExtinctionZeroMie;
	glm::vec4 betaExtinctionZeroRayleigh;
	glm::vec4 absorptionZeroOzone;
	float heightScaleRayleigh;
	float heightScaleMie;
	float layerHeightOzone;
	float layer0ConstantFactorOzone;
	float layer1ConstantFactorOzone;
	float heightRangeOzone;
	float maxHeight;
	float groundRadius;
};

class ScatteringLUTNode : public FramegraphNode {
  public:
	ScatteringLUTNode() { m_name = "Scattering LUT \"pre\"computation"; }

	virtual void create(FramegraphContext* context);

	virtual void setupResources(FramegraphContext* context) {}

	virtual void initResources(FramegraphContext* context);

	virtual void recordCommands(FramegraphContext* context, VkCommandBuffer targetCommandBuffer,
								const FramegraphNodeContext& nodeContext);

	virtual void handleWindowResize(FramegraphContext* context, uint32_t width, uint32_t height) {}

	virtual void destroy(FramegraphContext* context);

	FramegraphImageHandle transmittanceLUTHandle() { return m_transmittanceLUTHandle; }
	FramegraphImageHandle skyViewLUTHandle() { return m_skyViewLUTHandle; }
	FramegraphImageHandle aerialPerspectiveLUTHandle() { return m_aerialPerspectiveLUTHandle; }
	FramegraphImageHandle multiscatterLUTHandle() { return m_multiscatterLUTHandle; }

  private:
	FramegraphImageHandle m_transmittanceLUTHandle;
	FramegraphImageHandle m_skyViewLUTHandle;
	FramegraphImageHandle m_aerialPerspectiveLUTHandle;
	FramegraphImageHandle m_multiscatterLUTHandle;

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