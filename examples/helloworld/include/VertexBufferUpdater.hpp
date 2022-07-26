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
#pragma once

#include <Engine.hpp>
#include <graphics/GraphicsSubsystem.hpp>

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
