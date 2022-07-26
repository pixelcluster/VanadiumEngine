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

#include <graphics/framegraph/FramegraphNode.hpp>

class SwapchainClearNode : public vanadium::graphics::FramegraphNode {
  public:
	SwapchainClearNode() { m_name = "Swapchain clearing"; }

	void create(vanadium::graphics::FramegraphContext* context) override;

	void recordCommands(vanadium::graphics::FramegraphContext* context, VkCommandBuffer targetCommandBuffer,
						const vanadium::graphics::FramegraphNodeContext& nodeContext) override;

	void destroy(vanadium::graphics::FramegraphContext* context) override {}

  private:
};
