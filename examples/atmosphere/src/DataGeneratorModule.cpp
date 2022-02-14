#include <DataGeneratorModule.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <numbers>

#include <modules/gpu/helper/ErrorHelper.hpp>
#include <modules/gpu/helper/TextureImageLoader.hpp>
#include <volk.h>

DataGeneratorModule::DataGeneratorModule(VGPUModule* gpuModule, PlanetRenderNode* renderNode,
										 VWindowModule* windowModule) {
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

	// first circle around singularity point
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

	m_texHandle =
		loadTexture("./resources/earth_tex.jpg", &m_gpuModule->resourceAllocator(), &m_gpuModule->transferManager(),
					VK_IMAGE_USAGE_SAMPLED_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	m_seaMaskTexHandle =
		loadTexture("./resources/earth_bathymetry.jpg", &m_gpuModule->resourceAllocator(),
					&m_gpuModule->transferManager(), VK_IMAGE_USAGE_SAMPLED_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
					VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	VkSamplerCreateInfo samplerCreateInfo = { .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
											  .magFilter = VK_FILTER_LINEAR,
											  .minFilter = VK_FILTER_LINEAR,
											  .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
											  .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
											  .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
											  .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT };
	verifyResult(vkCreateSampler(m_gpuModule->context().device(), &samplerCreateInfo, nullptr, &m_textureSampler));

	VkDescriptorSetLayoutBinding uboBinding = { .binding = 0,
												.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
												.descriptorCount = 1,
												.stageFlags =
													VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT };
	VkDescriptorSetLayoutCreateInfo setLayoutCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, .bindingCount = 1, .pBindings = &uboBinding
	};
	verifyResult(vkCreateDescriptorSetLayout(m_gpuModule->context().device(), &setLayoutCreateInfo, nullptr,
											 &m_sceneDataSetLayout));

	VkDescriptorSetLayoutBinding texBindings[] = { { .binding = 0,
													 .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
													 .descriptorCount = 1,
													 .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
													 .pImmutableSamplers = &m_textureSampler },
												   { .binding = 1,
													 .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
													 .descriptorCount = 1,
													 .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
													 .pImmutableSamplers = &m_textureSampler } };
	VkDescriptorSetLayoutCreateInfo texSetLayoutCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, .bindingCount = 2, .pBindings = texBindings
	};
	verifyResult(vkCreateDescriptorSetLayout(m_gpuModule->context().device(), &texSetLayoutCreateInfo, nullptr,
											 &m_textureSetLayout));

	VkImageView view = m_gpuModule->resourceAllocator().requestImageView(
		m_texHandle, VImageResourceViewInfo{ .viewType = VK_IMAGE_VIEW_TYPE_2D,
											 .subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
																   .baseMipLevel = 0,
																   .levelCount = VK_REMAINING_MIP_LEVELS,
																   .baseArrayLayer = 0,
																   .layerCount = 1 } });
	VkImageView seaMaskView = m_gpuModule->resourceAllocator().requestImageView(
		m_seaMaskTexHandle, VImageResourceViewInfo{ .viewType = VK_IMAGE_VIEW_TYPE_2D,
													.subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
																		  .baseMipLevel = 0,
																		  .levelCount = VK_REMAINING_MIP_LEVELS,
																		  .baseArrayLayer = 0,
																		  .layerCount = 1 } });

	auto allocations = m_gpuModule->descriptorSetAllocator().allocateDescriptorSets(
		{ { .typeInfos = { { .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .count = 1 } },
			.layout = m_sceneDataSetLayout },
		  { .typeInfos = { { .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .count = 1 },
						   { .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .count = 1 } },
			.layout = m_textureSetLayout } });

	VkDescriptorBufferInfo info = { .buffer = m_gpuModule->resourceAllocator().nativeBufferHandle(
										m_gpuModule->transferManager().dstBufferHandle(m_sceneDataTransfer)),
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
	vkUpdateDescriptorSets(m_gpuModule->context().device(), 3, setWrites, 0, nullptr);

	renderNode->setupObjects(m_vertexBufferHandle, m_indexBufferHandle, m_sceneDataSetLayout, allocations[0].set,
							 m_textureSetLayout, allocations[1].set, totalIndexCount);
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

	constexpr float vFoV = glm::radians(65.0f);
	float tanHalfFoV = tanf(vFoV * 0.5);
	float aspectRatio = static_cast<float>(m_windowModule->width()) / static_cast<float>(m_windowModule->height());
	glm::vec3 camUp = glm::cross(directionCartesian, right);

	CameraSceneData sceneData = { .viewProjection = glm::perspective(vFoV, aspectRatio, 0.0f, 200.0f) *
													glm::lookAt(m_camPos, m_camPos + directionCartesian, camUp),
								  .camPos = glm::vec4(m_camPos, 1.0f) };
	m_gpuModule->transferManager().updateTransferData(m_sceneDataTransfer, &sceneData);
}

void DataGeneratorModule::onDeactivate(VEngine& engine) {
	vkDestroyDescriptorSetLayout(m_gpuModule->context().device(), m_sceneDataSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(m_gpuModule->context().device(), m_textureSetLayout, nullptr);
	vkDestroySampler(m_gpuModule->context().device(), m_textureSampler, nullptr);
}

void DataGeneratorModule::onDestroy(VEngine& engine) {
	free(m_pointBuffer);
	free(m_indexBuffer);
}