#include <DataGeneratorModule.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <numbers>

#include <graphics/helper/DescriptorSetAllocateInfoGenerator.hpp>
#include <graphics/helper/ErrorHelper.hpp>
#include <graphics/helper/TextureImageLoader.hpp>
#include <volk.h>

void forwardKey(uint32_t keyCode, uint32_t modifiers, vanadium::windowing::KeyState state, void* userData) {
	DataGenerator* generator = std::launder(reinterpret_cast<DataGenerator*>(userData));
	if (state == vanadium::windowing::KeyState::Released) {
		generator->setForwardState(false);
	} else
		generator->setForwardState(true);
}
void backKey(uint32_t keyCode, uint32_t modifiers, vanadium::windowing::KeyState state, void* userData) {
	DataGenerator* generator = std::launder(reinterpret_cast<DataGenerator*>(userData));
	if (state == vanadium::windowing::KeyState::Released) {
		generator->setBackState(false);
	} else
		generator->setBackState(true);
}
void rightKey(uint32_t keyCode, uint32_t modifiers, vanadium::windowing::KeyState state, void* userData) {
	DataGenerator* generator = std::launder(reinterpret_cast<DataGenerator*>(userData));
	if (state == vanadium::windowing::KeyState::Released) {
		generator->setRightState(false);
	} else
		generator->setRightState(true);
}
void leftKey(uint32_t keyCode, uint32_t modifiers, vanadium::windowing::KeyState state, void* userData) {
	DataGenerator* generator = std::launder(reinterpret_cast<DataGenerator*>(userData));
	if (state == vanadium::windowing::KeyState::Released) {
		generator->setLeftState(false);
	} else
		generator->setLeftState(true);
}
void mouseKey(uint32_t keyCode, uint32_t modifiers, vanadium::windowing::KeyState state, void* userData) {
	DataGenerator* generator = std::launder(reinterpret_cast<DataGenerator*>(userData));
	if (state == vanadium::windowing::KeyState::Released) {
		generator->mouseRelease();
	} else
		generator->mousePress();
}

DataGenerator::DataGenerator(vanadium::graphics::GraphicsSubsystem& subsystem,
							 vanadium::windowing::WindowInterface& interface, PlanetRenderNode* renderNode) {
	interface.addKeyListener(GLFW_KEY_W, 0,
							 vanadium::windowing::KeyState::Pressed | vanadium::windowing::KeyState::Released,
							 { .eventCallback = forwardKey,
							   .listenerDestroyCallback = vanadium::windowing::emptyListenerDestroyCallback,
							   .userData = this });
	interface.addKeyListener(GLFW_KEY_S, 0,
							 vanadium::windowing::KeyState::Pressed | vanadium::windowing::KeyState::Released,
							 { .eventCallback = backKey,
							   .listenerDestroyCallback = vanadium::windowing::emptyListenerDestroyCallback,
							   .userData = this });
	interface.addKeyListener(GLFW_KEY_A, 0,
							 vanadium::windowing::KeyState::Pressed | vanadium::windowing::KeyState::Released,
							 { .eventCallback = rightKey,
							   .listenerDestroyCallback = vanadium::windowing::emptyListenerDestroyCallback,
							   .userData = this });
	interface.addKeyListener(GLFW_KEY_D, 0,
							 vanadium::windowing::KeyState::Pressed | vanadium::windowing::KeyState::Released,
							 { .eventCallback = leftKey,
							   .listenerDestroyCallback = vanadium::windowing::emptyListenerDestroyCallback,
							   .userData = this });
	interface.addMouseKeyListener(GLFW_MOUSE_BUTTON_LEFT, 0,
								  vanadium::windowing::KeyState::Pressed | vanadium::windowing::KeyState::Released,
								  { .eventCallback = mouseKey,
									.listenerDestroyCallback = vanadium::windowing::emptyListenerDestroyCallback,
									.userData = this });

	m_pointBuffer = new VertexData[totalPointCount];
	m_indexBuffer = new uint32_t[totalIndexCount];

	m_pointBuffer[0].pos = glm::vec3(0.0f, 1.0f, 0.0f); // singularity point on top of sphere
	m_pointBuffer[0].texCoord = glm::vec2(0.5f, 0.0f);

	float theta = .0f;
	float dTheta = 1.0f * std::numbers::pi_v<float> / static_cast<float>(pointsPerLatitudeSegment);
	float phi = 0.0f;
	float dPhi = 2.0f * std::numbers::pi_v<float> / static_cast<float>(pointsPerLongitudeSegment);

	// first circle around singularity point
	for (size_t i = 0; i < pointsPerLatitudeSegment; ++i) {
		phi = 0.0f;
		theta -= dTheta;

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
		subsystem.context().resourceAllocator->createBuffer(bufferCreateInfo, {}, { .deviceLocal = true }, false);

	subsystem.context().transferManager->submitOneTimeTransfer(
		sizeof(VertexData) * totalPointCount, m_vertexBufferHandle, m_pointBuffer, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
		VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT);
	bufferCreateInfo = { .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
						 .size = sizeof(uint32_t) * totalIndexCount,
						 .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
						 .sharingMode = VK_SHARING_MODE_EXCLUSIVE };
	m_indexBufferHandle =
		subsystem.context().resourceAllocator->createBuffer(bufferCreateInfo, {}, { .deviceLocal = true }, false);

	subsystem.context().transferManager->submitOneTimeTransfer(sizeof(uint32_t) * totalIndexCount, m_indexBufferHandle,
															   m_indexBuffer, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
															   VK_ACCESS_INDEX_READ_BIT);

	m_sceneDataTransfer = subsystem.context().transferManager->createTransfer(
		sizeof(CameraSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
		VK_ACCESS_SHADER_READ_BIT);

	m_texHandle = loadTexture("./resources/earth_tex.jpg", subsystem.context().resourceAllocator,
							  subsystem.context().transferManager, VK_IMAGE_USAGE_SAMPLED_BIT,
							  VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT,
							  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	m_seaMaskTexHandle = loadTexture("./resources/earth_bathymetry.jpg", subsystem.context().resourceAllocator,
									 subsystem.context().transferManager, VK_IMAGE_USAGE_SAMPLED_BIT,
									 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT,
									 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	VkImageView view = subsystem.context().resourceAllocator->requestImageView(
		m_texHandle,
		vanadium::graphics::ImageResourceViewInfo{ .viewType = VK_IMAGE_VIEW_TYPE_2D,
												   .subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
																		 .baseMipLevel = 0,
																		 .levelCount = VK_REMAINING_MIP_LEVELS,
																		 .baseArrayLayer = 0,
																		 .layerCount = 1 } });
	VkImageView seaMaskView = subsystem.context().resourceAllocator->requestImageView(
		m_seaMaskTexHandle,
		vanadium::graphics::ImageResourceViewInfo{ .viewType = VK_IMAGE_VIEW_TYPE_2D,
												   .subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
																		 .baseMipLevel = 0,
																		 .levelCount = VK_REMAINING_MIP_LEVELS,
																		 .baseArrayLayer = 0,
																		 .layerCount = 1 } });

	m_planetPipelineID = subsystem.context().pipelineLibrary->findGraphicsPipeline("Planet Drawing");

	auto allocations = subsystem.context().descriptorSetAllocator->allocateDescriptorSets(
		vanadium::graphics::allocationInfosForPipeline(subsystem.context().pipelineLibrary,
													   vanadium::graphics::PipelineType::Graphics, m_planetPipelineID));

	VkDescriptorBufferInfo info = { .buffer = subsystem.context().resourceAllocator->nativeBufferHandle(
										subsystem.context().transferManager->dstBufferHandle(m_sceneDataTransfer)),
									.offset = 0,
									.range = VK_WHOLE_SIZE };

	VkDescriptorImageInfo imageInfo = { .imageView = view, .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
	VkDescriptorImageInfo seaMaskImageInfo = { .imageView = seaMaskView,
											   .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

	VkWriteDescriptorSet setWrites[3] = { { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
											.dstSet = allocations[0].set,
											.dstBinding = 0,
											.dstArrayElement = 0,
											.descriptorCount = 1,
											.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
											.pBufferInfo = &info },
										  { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
											.dstSet = allocations[1].set,
											.dstBinding = 0,
											.dstArrayElement = 0,
											.descriptorCount = 1,
											.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
											.pImageInfo = &imageInfo },
										  { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
											.dstSet = allocations[1].set,
											.dstBinding = 1,
											.dstArrayElement = 0,
											.descriptorCount = 1,
											.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
											.pImageInfo = &seaMaskImageInfo } };
	vkUpdateDescriptorSets(subsystem.context().deviceContext->device(), 3, setWrites, 0, nullptr);

	renderNode->setupObjects(m_vertexBufferHandle, m_indexBufferHandle, m_sceneDataSetLayout, allocations[0].set,
							 m_textureSetLayout, allocations[1].set, totalIndexCount);
}

void DataGenerator::update(vanadium::graphics::GraphicsSubsystem& subsystem,
						   vanadium::windowing::WindowInterface& interface) {
	float sinTheta = sinf(m_camTheta);
	float cosTheta = cosf(m_camTheta);
	float sinPhi = sinf(m_camPhi);
	float cosPhi = cosf(m_camPhi);

	glm::vec3 directionCartesian = glm::vec3(cosPhi * sinTheta, cosTheta, -sinPhi * sinTheta);
	glm::vec3 right = glm::vec3(sinPhi, 0.0f, cosPhi);
	glm::vec3 camUp = glm::cross(directionCartesian, right);

	if (m_movingForward) {
		m_camPos += directionCartesian * interface.deltaTime();
	}
	if (m_movingBack) {
		m_camPos -= directionCartesian * interface.deltaTime();
	}
	if (m_movingRight) {
		m_camPos += right * interface.deltaTime();
	}
	if (m_movingLeft) {
		m_camPos -= right * interface.deltaTime();
	}

	if (m_mousePressed) {
		vanadium::Vector2 mousePos = interface.mousePos();
		if (m_lastMouseValid) {
			float deltaX = mousePos.x - m_lastMouseX;
			float deltaY = mousePos.y - m_lastMouseY;

			m_camPhi += deltaX * 0.2f * interface.deltaTime();
			m_camTheta += deltaY * 0.2f * interface.deltaTime();
		}
		m_lastMouseX = mousePos.x;
		m_lastMouseY = mousePos.y;
		m_lastMouseValid = true;
	} else
		m_lastMouseValid = false;

	constexpr float vFoV = glm::radians(65.0f);
	uint32_t width, height;
	interface.windowSize(width, height);
	float aspectRatio = static_cast<float>(width) / static_cast<float>(height);

	CameraSceneData sceneData = { .viewProjection = glm::perspective(vFoV, aspectRatio, 0.0f, 200.0f) *
													glm::lookAt(m_camPos, m_camPos + directionCartesian, camUp),
								  .camPos = glm::vec4(m_camPos, 1.0f) };
	subsystem.context().transferManager->updateTransferData(m_sceneDataTransfer, subsystem.frameIndex(), &sceneData);
}

void DataGenerator::destroy(vanadium::graphics::GraphicsSubsystem& subsystem) {
	delete[] m_pointBuffer;
	delete[] m_indexBuffer;
}