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
		-0.5f, 0.5f,
		1.0f, 0.0f, 0.0f,
		0.5f, 0.5f,
		0.0f, 1.0f, 0.0f,
		0.0f, -0.5f,
		0.0f, 0.0f, 1.0f
	};
	// clang-format on
	m_gpuModule->transferManager().updateTransferData(m_vertexDataTransfer, vertexData);
}

void VertexBufferUpdateModule::onDeactivate(VEngine& engine) {}

void VertexBufferUpdateModule::onDestroy(VEngine& engine) {}

void VertexBufferUpdateModule::onDependentModuleDeactivate(VEngine& engine, VModule* moduleToDestroy) {
	engine.destroyModule(this);
}
