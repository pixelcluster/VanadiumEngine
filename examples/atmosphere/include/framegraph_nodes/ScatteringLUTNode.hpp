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

#include <glm/glm.hpp>
#include <graphics/framegraph/FramegraphNode.hpp>

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

struct Sphere {
	glm::vec2 center;
	float radiusSquared;
	float _pad;
};

struct SkyViewComputeData {
	TransmittanceComputeData commonData;
	float currentHeight;
	glm::vec3 _pad = {};
	glm::vec4 rayleighScattering;
	float mieScattering;
	glm::vec3 _pad2 = {};
	Sphere sunSphere;
	glm::vec4 sunLuminance;
};

class ScatteringLUTNode : public vanadium::graphics::FramegraphNode {
  public:
	ScatteringLUTNode() { m_name = "Scattering LUT \"pre\"computation"; }

	void create(vanadium::graphics::FramegraphContext* context) override;

	void afterResourceInit(vanadium::graphics::FramegraphContext* context) override;

	void recordCommands(vanadium::graphics::FramegraphContext* context, VkCommandBuffer targetCommandBuffer,
								const vanadium::graphics::FramegraphNodeContext& nodeContext) override;

	void recreateSwapchainResources(vanadium::graphics::FramegraphContext* context, uint32_t width,
									uint32_t height) override {}

	void destroy(vanadium::graphics::FramegraphContext* context) override;

	[[nodiscard]] vanadium::graphics::FramegraphImageHandle transmittanceLUTHandle() const {
		return m_transmittanceLUTHandle;
	}
	[[nodiscard]] vanadium::graphics::FramegraphImageHandle skyViewLUTHandle() const { return m_skyViewLUTHandle; }
	[[nodiscard]] vanadium::graphics::FramegraphImageHandle aerialPerspectiveLUTHandle() const {
		return m_aerialPerspectiveLUTHandle;
	}
	[[nodiscard]] vanadium::graphics::FramegraphImageHandle multiscatterLUTHandle() const {
		return m_multiscatterLUTHandle;
	}

  private:
	vanadium::graphics::FramegraphImageHandle m_transmittanceLUTHandle = {};
	vanadium::graphics::FramegraphImageHandle m_skyViewLUTHandle = {};
	vanadium::graphics::FramegraphImageHandle m_aerialPerspectiveLUTHandle = {};
	vanadium::graphics::FramegraphImageHandle m_multiscatterLUTHandle = {};

	vanadium::graphics::BufferResourceHandle m_skyViewInputBufferHandle;

	uint32_t m_transmittanceComputationID = ~0U;
	uint32_t m_skyViewComputationID = ~0U;

	VkDescriptorSet m_transmittanceComputationOutputSet = {};
	VkDescriptorSet m_skyViewComputationInputSets[vanadium::graphics::frameInFlightCount] = {};
	VkDescriptorSet m_skyViewComputationOutputSet = {};
	VkDescriptorSet m_aerialPerspectiveComputationOutputSet = {};
	VkDescriptorSet m_multiscatterComputationOutputSet = {};

	VkDescriptorSetLayout m_lutSetLayout = {};
	VkDescriptorSet m_lutSet = {};
};
