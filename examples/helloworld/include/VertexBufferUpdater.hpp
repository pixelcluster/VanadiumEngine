#pragma once

#include <Engine.hpp>

class VertexBufferUpdater {
  public:
	void init(vanadium::graphics::GraphicsSubsystem& graphicsSubsystem);

	void update(vanadium::graphics::GraphicsSubsystem& graphicsSubsystem, float elapsedTime);

	vanadium::graphics::BufferResourceHandle vertexBufferHandle(const vanadium::graphics::RenderContext& context) const {
		return context.transferManager->dstBufferHandle(m_vertexDataTransfer);
	}
  private:
	vanadium::graphics::GPUTransferHandle m_vertexDataTransfer;
};