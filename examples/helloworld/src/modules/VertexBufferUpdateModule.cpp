#include <modules/VertexBufferUpdateModule.hpp>

VertexBufferUpdateModule::VertexBufferUpdateModule(VGPUModule* gpuModule, VWindowModule* windowModule)
	: m_gpuModule(gpuModule), m_windowModule(windowModule) {
	m_vertexDataTransfer = gpuModule->transferManager().createTransfer(
		15 * sizeof(float), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
		VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT);
}

void VertexBufferUpdateModule::onCreate(VEngine& engine) {
}

void VertexBufferUpdateModule::onActivate(VEngine& engine) {}

void VertexBufferUpdateModule::onExecute(VEngine& engine) {
	// clang-format off
	float vertexData[15] = {
		-0.5f, 0.3,
		1.0f, 0.0f, 0.0f,
		0.5f, 0.3f,
		0.0f, 1.0f, 0.0f,
		0.0f, -0.7f,
		0.0f, 0.0f, 1.0f
	};

	for (size_t i = 0; i < 3; ++i) {
		float x = vertexData[i * 5];
		float y = vertexData[i * 5 + 1];

		vertexData[i * 5] = (x * cos(m_timeCounter) - y * sin(m_timeCounter)) / (static_cast<float>(m_windowModule->width()) /  static_cast<float>(m_windowModule->height()));
		vertexData[i * 5 + 1] = x * sin(m_timeCounter) + y * cos(m_timeCounter);
		vertexData[i * 5] += sin(m_timeCounter * 2.0f);
	}

	// clang-format on
	m_gpuModule->transferManager().updateTransferData(m_vertexDataTransfer, vertexData);

	m_timeCounter += m_windowModule->deltaTime() * 4.0f;
}

void VertexBufferUpdateModule::onDeactivate(VEngine& engine) {}

void VertexBufferUpdateModule::onDestroy(VEngine& engine) {}

void VertexBufferUpdateModule::onDependentModuleDeactivate(VEngine& engine, VModule* moduleToDestroy) {
	engine.destroyModule(this);
}
