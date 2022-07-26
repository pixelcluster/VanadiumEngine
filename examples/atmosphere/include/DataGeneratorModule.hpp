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

#include <Engine.hpp>
#include <framegraph_nodes/PlanetRenderNode.hpp>
#include <glm/glm.hpp>
#include <graphics/util/GPUTransferManager.hpp>
#include <numbers>

constexpr size_t pointsPerLatitudeSegment = 256;
constexpr size_t pointsPerLongitudeSegment = 256;
constexpr size_t individualPointsPerLongitudeSegment = pointsPerLongitudeSegment + 1;
constexpr size_t totalPointCount = pointsPerLatitudeSegment * individualPointsPerLongitudeSegment +
								   2; // add 2 singularity points at the top of the sphere and at the bottom
constexpr size_t totalIndexCount = individualPointsPerLongitudeSegment * 6 * pointsPerLongitudeSegment;

struct CameraSceneData {
	glm::mat4 viewProjection;
	glm::vec4 camPos;
};

struct VertexData {
	glm::vec3 pos;
	glm::vec2 texCoord;
};

class DataGenerator {
  public:
	DataGenerator(vanadium::graphics::GraphicsSubsystem& subsystem, vanadium::windowing::WindowInterface& interface,
				  PlanetRenderNode* renderNode);

	void update(vanadium::graphics::GraphicsSubsystem& subsystem, vanadium::windowing::WindowInterface& interface);
	void destroy(vanadium::graphics::GraphicsSubsystem& subsystem);

	void setForwardState(bool isMoving) { m_movingForward = isMoving; }
	void setBackState(bool isMoving) { m_movingBack = isMoving; }
	void setLeftState(bool isMoving) { m_movingLeft = isMoving; }
	void setRightState(bool isMoving) { m_movingRight = isMoving; }
	void mousePress() { m_mousePressed = true; }
	void mouseRelease() { m_mousePressed = false; }

  private:
	PlanetRenderNode* m_renderNode;

	uint32_t m_planetPipelineID;

	glm::vec3 m_camPos = glm::vec3(0.0f, 0.0f, -5.0f);
	glm::vec3 m_deltaPos = glm::vec3(0.0f);
	bool m_movingForward = false;
	bool m_movingBack = false;
	bool m_movingLeft = false;
	bool m_movingRight = false;
	bool m_mousePressed = false;
	float m_camPhi = -0.5 * std::numbers::pi_v<float>;
	float m_camTheta = 0.5 * std::numbers::pi_v<float>;

	float m_lastMouseX;
	float m_lastMouseY;
	bool m_lastMouseValid = false;

	VertexData* m_pointBuffer;
	uint32_t* m_indexBuffer;

	vanadium::graphics::BufferResourceHandle m_vertexBufferHandle;
	vanadium::graphics::BufferResourceHandle m_indexBufferHandle;
	vanadium::graphics::ImageResourceHandle m_texHandle;
	vanadium::graphics::ImageResourceHandle m_seaMaskTexHandle;
	vanadium::graphics::GPUTransferHandle m_sceneDataTransfer;

	VkDescriptorSetLayout m_sceneDataSetLayout;
	VkDescriptorSetLayout m_textureSetLayout;
	VkSampler m_textureSampler;
};
