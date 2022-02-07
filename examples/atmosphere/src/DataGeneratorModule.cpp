#include <DataGeneratorModule.hpp>
#include <numbers>

DataGeneratorModule::DataGeneratorModule(VGPUModule* gpuModule) {
	m_gpuModule = gpuModule;
	m_pointBuffer = reinterpret_cast<Vector3*>(malloc(sizeof(Vector3) * totalPointCount));
	m_indexBuffer = reinterpret_cast<uint32_t*>(malloc(sizeof(uint32_t) * totalPointCount));

	m_pointBuffer[0] = Vector3(0.0f, 1.0f, 0.0f); // singularity point on top of sphere

	float theta = std::numbers::pi_v<float> * 0.5f;
	float dTheta = 2.0f * std::numbers::pi_v<float> / static_cast<float>(pointsPerLatitudeSegment);
	float phi = 0.0f;
	float dPhi = 2.0f * std::numbers::pi_v<float> / static_cast<float>(pointsPerLongitudeSegment);

	// first circle around degenerate point
	for (size_t i = 0; i < pointsPerLatitudeSegment; ++i) {
		float sinTheta = sinf(theta);
		float cosTheta = cosf(theta);
		for (size_t j = 0; j < pointsPerLatitudeSegment; ++j) {
			size_t index = i * pointsPerLongitudeSegment + j + 1;

			float sinPhi = sinf(phi);
			float cosPhi = cosf(phi);

			m_pointBuffer[index] = Vector3(cosPhi * sinTheta, cosTheta, -sinPhi * sinTheta);
			phi += dPhi;
		}
		theta -= dTheta;
	}

	m_pointBuffer[totalPointCount - 1] = Vector3(0.0f, -1.0f, 0.0f);

	VkBufferCreateInfo bufferCreateInfo = { .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
											.size = sizeof(Vector3) * totalPointCount,
											.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
											.sharingMode = VK_SHARING_MODE_EXCLUSIVE };
	m_vertexBufferHandle =
		m_gpuModule->resourceAllocator().createBuffer(bufferCreateInfo, {}, { .deviceLocal = true }, true);

	m_gpuModule->transferManager().submitOneTimeTransfer(sizeof(Vector3) * totalPointCount, m_vertexBufferHandle,
														 m_pointBuffer, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
														 VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT);
}

void DataGeneratorModule::onCreate(VEngine& engine) {}

void DataGeneratorModule::onActivate(VEngine& engine) {}

void DataGeneratorModule::onExecute(VEngine& engine) {}

void DataGeneratorModule::onDeactivate(VEngine& engine) {}

void DataGeneratorModule::onDestroy(VEngine& engine) {
	free(m_pointBuffer);
	free(m_indexBuffer);
}