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

#include <graphics/framegraph/FramegraphContext.hpp>

namespace vanadium::graphics {

	class FramegraphNode {
	  public:
		virtual ~FramegraphNode() {}
		virtual void create(FramegraphContext* context) = 0;

		virtual void afterResourceInit(FramegraphContext* context) {}

		virtual void recordCommands(FramegraphContext* context, VkCommandBuffer targetCommandBuffer,
									const FramegraphNodeContext& nodeContext) = 0;

		virtual void recreateSwapchainResources(FramegraphContext* context, uint32_t width, uint32_t height) {}

		virtual void destroy(FramegraphContext* context) = 0;

		const std::string& name() const { return m_name; }

	  protected:
		std::string m_name;
	};

} // namespace vanadium::graphics
