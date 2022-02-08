#include <DataGeneratorModule.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <numbers>

DataGeneratorModule::DataGeneratorModule(VGPUModule* gpuModule, VWindowModule* windowModule) {
	m_gpuModule = gpuModule;
	m_windowModule = windowModule;
	m_pointBuffer = reinterpret_cast<glm::vec3*>(malloc(sizeof(glm::vec3) * totalPointCount));
	m_indexBuffer = reinterpret_cast<uint32_t*>(malloc(sizeof(uint32_t) * totalIndexCount));

	m_pointBuffer[0] = glm::vec3(0.0f, 1.0f, 0.0f); // singularity point on top of sphere

	float theta = .0f;
	float dTheta = 1.0f * std::numbers::pi_v<float> / static_cast<float>(pointsPerLatitudeSegment);
	float phi = 0.0f;
	float dPhi = 2.0f * std::numbers::pi_v<float> / static_cast<float>(pointsPerLongitudeSegment);

	// first circle around degenerate point
	for (size_t i = 0; i < pointsPerLatitudeSegment; ++i) {
		float sinTheta = sinf(theta);
		float cosTheta = cosf(theta);
		for (size_t j = 0; j < pointsPerLongitudeSegment; ++j) {
			size_t index = i * pointsPerLongitudeSegment + j + 1;

			float sinPhi = sinf(phi);
			float cosPhi = cosf(phi);

			m_pointBuffer[index] = glm::vec3(cosPhi * sinTheta, cosTheta, -sinPhi * sinTheta);
			phi += dPhi;
		}
		theta -= dTheta;
	}

	m_pointBuffer[totalPointCount - 1] = glm::vec3(0.0f, -1.0f, 0.0f);

	for (size_t i = 0; i < pointsPerLongitudeSegment; ++i) {
		uint32_t secondTriangleIndex = i + 1;
		if (secondTriangleIndex == pointsPerLongitudeSegment)
			secondTriangleIndex = 0;
		m_indexBuffer[i * 3] = i;
		m_indexBuffer[i * 3 + 1] = 0;
		m_indexBuffer[i * 3 + 2] = secondTriangleIndex;
	}

	uint32_t indexOffset = pointsPerLongitudeSegment + 1;
	uint32_t indexBufferOffset = pointsPerLongitudeSegment * 3;

	for (size_t i = 0; i < pointsPerLatitudeSegment - 1; ++i) {
		for (size_t j = 0; j < pointsPerLongitudeSegment; ++j) {
			uint32_t nextIndexOffset = indexOffset + pointsPerLongitudeSegment * 6;
			uint32_t thirdPointIndex = indexOffset + j + 1;
			uint32_t nextLatitudeThirdPointIndex = nextIndexOffset + j + 1;

			if (j + 1 == pointsPerLongitudeSegment) {
				thirdPointIndex = indexOffset;
				nextLatitudeThirdPointIndex = nextIndexOffset;
			}

			m_indexBuffer[indexBufferOffset + j * 6] = nextIndexOffset + j;
			m_indexBuffer[indexBufferOffset + j * 6 + 1] = nextLatitudeThirdPointIndex;
			m_indexBuffer[indexBufferOffset + j * 6 + 2] = j;
			m_indexBuffer[indexBufferOffset + j * 6 + 3] = nextLatitudeThirdPointIndex;
			m_indexBuffer[indexBufferOffset + j * 6 + 4] = thirdPointIndex;
			m_indexBuffer[indexBufferOffset + j * 6 + 5] = j;
		}

		indexOffset += pointsPerLongitudeSegment;
		indexBufferOffset += pointsPerLongitudeSegment * 6;
	}

	for (size_t i = 0; i < pointsPerLongitudeSegment; ++i) {
		uint32_t secondTriangleIndex = i + 1;
		if (secondTriangleIndex == pointsPerLongitudeSegment)
			secondTriangleIndex = totalPointCount - 1;

		m_indexBuffer[indexBufferOffset + i * 3] = indexOffset + i;
		m_indexBuffer[indexBufferOffset + i * 3 + 1] = totalPointCount - 1;
		m_indexBuffer[indexBufferOffset + i * 3 + 2] = secondTriangleIndex;
	}

	VkBufferCreateInfo bufferCreateInfo = { .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
											.size = sizeof(glm::vec3) * totalPointCount,
											.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
											.sharingMode = VK_SHARING_MODE_EXCLUSIVE };
	m_vertexBufferHandle =
		m_gpuModule->resourceAllocator().createBuffer(bufferCreateInfo, {}, { .deviceLocal = true }, true);

	m_gpuModule->transferManager().submitOneTimeTransfer(sizeof(glm::vec3) * totalPointCount, m_vertexBufferHandle,
														 m_pointBuffer, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
														 VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT);
	bufferCreateInfo = { .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
											.size = sizeof(uint32_t) * totalIndexCount,
											.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
											.sharingMode = VK_SHARING_MODE_EXCLUSIVE };
	m_indexBufferHandle =
		m_gpuModule->resourceAllocator().createBuffer(bufferCreateInfo, {}, { .deviceLocal = true }, true);

	m_gpuModule->transferManager().submitOneTimeTransfer(sizeof(uint32_t) * totalIndexCount, m_indexBufferHandle,
														 m_indexBuffer, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
														 VK_ACCESS_INDEX_READ_BIT);

	m_sceneDataTransfer =
		m_gpuModule->transferManager().createTransfer(sizeof(CameraSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
													  VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT);
}

void DataGeneratorModule::onCreate(VEngine& engine) {}

void DataGeneratorModule::onActivate(VEngine& engine) {}

void DataGeneratorModule::onExecute(VEngine& engine) {
	float sinTheta = sinf(m_camTheta);
	float cosTheta = cosf(m_camTheta);
	float sinPhi = sinf(m_camPhi);
	float cosPhi = cosf(m_camPhi);
	glm::vec3 directionCartesian = glm::vec3(cosPhi * sinTheta, cosTheta, -sinPhi * sinTheta);
	glm::vec3 right = glm::vec3(sinPhi, 0.0f, cosPhi);

	if (m_windowModule->keyCharState('W') != VKeyState::Released) {
		m_camPos += directionCartesian * 2.0f * static_cast<float>(m_windowModule->deltaTime());
	}
	if (m_windowModule->keyCharState('S') != VKeyState::Released) {
		m_camPos -= directionCartesian * 2.0f * static_cast<float>(m_windowModule->deltaTime());
	}
	if (m_windowModule->keyCharState('A') != VKeyState::Released) {
		m_camPos -= right * 2.0f * static_cast<float>(m_windowModule->deltaTime());
	}
	if (m_windowModule->keyCharState('D') != VKeyState::Released) {
		m_camPos += right * 2.0f * static_cast<float>(m_windowModule->deltaTime());
	}

	switch (m_windowModule->mouseKeyState(0)) {
		case VKeyState::Pressed:
			if (m_lastMouseValid) {
				float deltaX = m_windowModule->mouseX() - m_lastMouseX;
				float deltaY = m_windowModule->mouseY() - m_lastMouseY;

				m_camPhi -= deltaX * 0.2f * static_cast<float>(m_windowModule->deltaTime());
				m_camTheta -= deltaY * 0.2f * static_cast<float>(m_windowModule->deltaTime());
			}
			m_lastMouseX = m_windowModule->mouseX();
			m_lastMouseY = m_windowModule->mouseY();
			m_lastMouseValid = true;
			break;
		default:
			m_lastMouseValid = false;
			break;
	}

	CameraSceneData sceneData = { .viewProjection = glm::perspective(glm::radians(65.0f),
																	 static_cast<float>(m_windowModule->width()) /
																		 static_cast<float>(m_windowModule->height()),
																	 0.0f, 200.0f) *
													glm::lookAt(m_camPos, m_camPos + directionCartesian,
																glm::cross(right, directionCartesian)) };
	m_gpuModule->transferManager().updateTransferData(m_sceneDataTransfer, &sceneData);
}

void DataGeneratorModule::onDeactivate(VEngine& engine) {}

void DataGeneratorModule::onDestroy(VEngine& engine) {
	free(m_pointBuffer);
	free(m_indexBuffer);
}