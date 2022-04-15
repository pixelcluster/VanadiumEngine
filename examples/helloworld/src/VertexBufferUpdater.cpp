#include <VertexBufferUpdater.hpp>
#include <cmath>

void VertexBufferUpdater::init(vanadium::graphics::GraphicsSubsystem& graphicsSubsystem) {
	m_vertexDataTransfer = graphicsSubsystem.context().transferManager->createTransfer(
		15 * sizeof(float), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
		VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT);
}

void VertexBufferUpdater::update(vanadium::graphics::GraphicsSubsystem& graphicsSubsystem, float elapsedTime) {
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

		vertexData[i * 5] = (x * cosf(elapsedTime) - y * sinf(elapsedTime)) / (static_cast<float>(graphicsSubsystem.context().targetSurface->properties().width) /  static_cast<float>(graphicsSubsystem.context().targetSurface->properties().height));
		vertexData[i * 5 + 1] = x * sinf(elapsedTime) + y * cosf(elapsedTime);
		vertexData[i * 5] += sinf(elapsedTime * 2.0f);
	}

	// clang-format on
	graphicsSubsystem.context().transferManager->updateTransferData(m_vertexDataTransfer, graphicsSubsystem.frameIndex(), vertexData);
}
