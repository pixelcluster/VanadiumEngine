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

class ScatteringLUTNode : public vanadium::graphics::FramegraphNode {
  public:
	ScatteringLUTNode() { m_name = "Scattering LUT \"pre\"computation"; }

	virtual void create(vanadium::graphics::FramegraphContext* context);

	virtual void setupResources(vanadium::graphics::FramegraphContext* context);

	virtual void initResources(vanadium::graphics::FramegraphContext* context);

	virtual void recordCommands(vanadium::graphics::FramegraphContext* context, VkCommandBuffer targetCommandBuffer,
								const vanadium::graphics::FramegraphNodeContext& nodeContext);

	virtual void handleWindowResize(vanadium::graphics::FramegraphContext* context, uint32_t width, uint32_t height) {}

	virtual void destroy(vanadium::graphics::FramegraphContext* context);

	vanadium::graphics::FramegraphImageHandle transmittanceLUTHandle() { return m_transmittanceLUTHandle; }
	vanadium::graphics::FramegraphImageHandle skyViewLUTHandle() { return m_skyViewLUTHandle; }
	vanadium::graphics::FramegraphImageHandle aerialPerspectiveLUTHandle() { return m_aerialPerspectiveLUTHandle; }
	vanadium::graphics::FramegraphImageHandle multiscatterLUTHandle() { return m_multiscatterLUTHandle; }

  private:
	vanadium::graphics::FramegraphImageHandle m_transmittanceLUTHandle;
	vanadium::graphics::FramegraphImageHandle m_skyViewLUTHandle;
	vanadium::graphics::FramegraphImageHandle m_aerialPerspectiveLUTHandle;
	vanadium::graphics::FramegraphImageHandle m_multiscatterLUTHandle;

	uint32_t m_transmittanceComputationID;

	VkDescriptorSet m_transmittanceComputationOutputSet;
	VkDescriptorSet m_skyViewComputationOutputSet;
	VkDescriptorSet m_aerialPerspectiveComputationOutputSet;
	VkDescriptorSet m_multiscatterComputationOutputSet;

	VkDescriptorSetLayout m_lutSetLayout;
	VkDescriptorSet m_lutSet;
};