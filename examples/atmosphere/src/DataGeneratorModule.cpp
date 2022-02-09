#include <DataGeneratorModule.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <numbers>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

DataGeneratorModule::DataGeneratorModule(VGPUModule* gpuModule, VWindowModule* windowModule) {
	m_gpuModule = gpuModule;
	m_windowModule = windowModule;
	m_pointBuffer = reinterpret_cast<VertexData*>(malloc(sizeof(VertexData) * totalPointCount));
	m_indexBuffer = reinterpret_cast<uint32_t*>(malloc(sizeof(uint32_t) * totalIndexCount));

	m_pointBuffer[0].pos = glm::vec3(0.0f, 1.0f, 0.0f); // singularity point on top of sphere
	m_pointBuffer[0].texCoord = glm::vec2(0.5f, 0.0f);

	float theta = .0f;
	float dTheta = 1.0f * std::numbers::pi_v<float> / static_cast<float>(pointsPerLatitudeSegment);
	float phi = 0.0f;
	float dPhi = 2.0f * std::numbers::pi_v<float> / static_cast<float>(pointsPerLongitudeSegment);

	// first circle around degenerate point
	for (size_t i = 0; i < pointsPerLatitudeSegment; ++i) {
		phi = 0.0f;
		theta -= dTheta;

		float latitude = 0.5f * std::numbers::pi_v<float> + theta;

		float sinTheta = sinf(theta);
		float cosTheta = cosf(theta);
		for (size_t j = 0; j < individualPointsPerLongitudeSegment; ++j) {
			size_t index = i * individualPointsPerLongitudeSegment + j + 1;

			float sinPhi = sinf(phi);
			float cosPhi = cosf(phi);

			m_pointBuffer[index].pos = glm::vec3(cosPhi * sinTheta, cosTheta, -sinPhi * sinTheta);
		

			m_pointBuffer[index].texCoord.x = 1.0f - phi / (2.0f * std::numbers::pi_v<float>);
			m_pointBuffer[index].texCoord.y = -theta / std::numbers::pi_v<float>;
			phi += dPhi;
		}
	}

	m_pointBuffer[totalPointCount - 1].pos = glm::vec3(0.0f, -1.0f, 0.0f);
	m_pointBuffer[totalPointCount - 1].texCoord = glm::vec2(0.5f, 1.0f);

	for (size_t i = 0; i < individualPointsPerLongitudeSegment; ++i) {
		uint32_t firstTriangleIndex = i;
		if (firstTriangleIndex == 0)
			firstTriangleIndex = individualPointsPerLongitudeSegment;

		m_indexBuffer[i * 3] = firstTriangleIndex;
		m_indexBuffer[i * 3 + 1] = 0;
		m_indexBuffer[i * 3 + 2] = i + 1;
	}

	uint32_t indexOffset = 1;
	uint32_t indexBufferOffset = individualPointsPerLongitudeSegment * 3;

	for (size_t i = 0; i < pointsPerLatitudeSegment - 1; ++i) {
		for (size_t j = 0; j < individualPointsPerLongitudeSegment; ++j) {
			uint32_t nextIndexOffset = indexOffset + individualPointsPerLongitudeSegment;
			uint32_t thirdPointIndex = indexOffset + j + 1;
			uint32_t nextLatitudeThirdPointIndex = nextIndexOffset + j + 1;

			if (j + 1 == individualPointsPerLongitudeSegment) {
				thirdPointIndex = indexOffset;
				nextLatitudeThirdPointIndex = nextIndexOffset;
			}

			m_indexBuffer[indexBufferOffset + j * 6] = nextIndexOffset + j;
			m_indexBuffer[indexBufferOffset + j * 6 + 1] = indexOffset + j;
			m_indexBuffer[indexBufferOffset + j * 6 + 2] = nextLatitudeThirdPointIndex;
			m_indexBuffer[indexBufferOffset + j * 6 + 3] = nextLatitudeThirdPointIndex;
			m_indexBuffer[indexBufferOffset + j * 6 + 4] = indexOffset + j;
			m_indexBuffer[indexBufferOffset + j * 6 + 5] = thirdPointIndex;
		}

		indexOffset += individualPointsPerLongitudeSegment;
		indexBufferOffset += individualPointsPerLongitudeSegment * 6;
	}

	for (size_t i = 0; i < individualPointsPerLongitudeSegment; ++i) {
		uint32_t secondTriangleIndex = indexOffset + i + 1;
		if (secondTriangleIndex == individualPointsPerLongitudeSegment)
			secondTriangleIndex = totalPointCount - 1;

		m_indexBuffer[indexBufferOffset + i * 3] = indexOffset + i;
		m_indexBuffer[indexBufferOffset + i * 3 + 1] = secondTriangleIndex;
		m_indexBuffer[indexBufferOffset + i * 3 + 2] = totalPointCount - 1;
	}

	VkBufferCreateInfo bufferCreateInfo = { .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
											.size = sizeof(VertexData) * totalPointCount,
											.usage =
												VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
											.sharingMode = VK_SHARING_MODE_EXCLUSIVE };
	m_vertexBufferHandle =
		m_gpuModule->resourceAllocator().createBuffer(bufferCreateInfo, {}, { .deviceLocal = true }, false);

	m_gpuModule->transferManager().submitOneTimeTransfer(sizeof(VertexData) * totalPointCount, m_vertexBufferHandle,
														 m_pointBuffer, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
														 VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT);
	bufferCreateInfo = { .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
						 .size = sizeof(uint32_t) * totalIndexCount,
						 .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
						 .sharingMode = VK_SHARING_MODE_EXCLUSIVE };
	m_indexBufferHandle =
		m_gpuModule->resourceAllocator().createBuffer(bufferCreateInfo, {}, { .deviceLocal = true }, false);

	m_gpuModule->transferManager().submitOneTimeTransfer(sizeof(uint32_t) * totalIndexCount, m_indexBufferHandle,
														 m_indexBuffer, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
														 VK_ACCESS_INDEX_READ_BIT);

	m_sceneDataTransfer =
		m_gpuModule->transferManager().createTransfer(sizeof(CameraSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
													  VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT);

	int channels = 4;

	int imageWidth, imageHeight;

	void* data = stbi_load("./resources/earth_tex.jpg", &imageWidth, &imageHeight, &channels, channels);

	VkImageCreateInfo imageCreateInfo = { .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
										  .imageType = VK_IMAGE_TYPE_2D,
										  .format = VK_FORMAT_R8G8B8A8_SRGB,
										  .extent = { .width = static_cast<uint32_t>(imageWidth),
													  .height = static_cast<uint32_t>(imageHeight),
													  .depth = 1U },
										  .mipLevels = 1U,
										  .arrayLayers = 1U,
										  .samples = VK_SAMPLE_COUNT_1_BIT,
										  .tiling = VK_IMAGE_TILING_OPTIMAL,
										  .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
										  .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
										  .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED };

	m_texHandle = m_gpuModule->resourceAllocator().createImage(imageCreateInfo, {}, { .deviceLocal = true });

	m_gpuModule->transferManager().submitImageTransfer(m_texHandle,
													   { .imageSubresource = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
																			   .mipLevel = 0,
																			   .baseArrayLayer = 0,
																			   .layerCount = 1 },
														 .imageExtent = { .width = static_cast<uint32_t>(imageWidth),
																		  .height = static_cast<uint32_t>(imageHeight),
																		  .depth = 1U } },
													   data, imageWidth * imageHeight * 4,
													   VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT,
													   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
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
		m_camPos += right * 2.0f * static_cast<float>(m_windowModule->deltaTime());
	}
	if (m_windowModule->keyCharState('D') != VKeyState::Released) {
		m_camPos -= right * 2.0f * static_cast<float>(m_windowModule->deltaTime());
	}

	switch (m_windowModule->mouseKeyState(0)) {
		case VKeyState::Pressed:
			if (m_lastMouseValid) {
				float deltaX = m_windowModule->mouseX() - m_lastMouseX;
				float deltaY = m_windowModule->mouseY() - m_lastMouseY;

				m_camPhi += deltaX * 0.2f * static_cast<float>(m_windowModule->deltaTime());
				m_camTheta += deltaY * 0.2f * static_cast<float>(m_windowModule->deltaTime());
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
																glm::cross(directionCartesian, right)) };
	m_gpuModule->transferManager().updateTransferData(m_sceneDataTransfer, &sceneData);
}

void DataGeneratorModule::onDeactivate(VEngine& engine) {}

void DataGeneratorModule::onDestroy(VEngine& engine) {
	free(m_pointBuffer);
	free(m_indexBuffer);
}