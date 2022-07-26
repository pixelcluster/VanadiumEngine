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
