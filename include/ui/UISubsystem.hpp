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

#include <ui/Control.hpp>
#include <ui/FontLibrary.hpp>
#include <ui/UIRendererNode.hpp>
#include <windowing/WindowInterface.hpp>

namespace vanadium::ui {
	class UISubsystem {
	  public:
		UISubsystem(windowing::WindowInterface* windowInterface, const graphics::RenderContext& context,
					const std::string_view& fontLibraryFile, const Vector4& clearValue);

		template <RenderableShape T, typename... Args>
		requires(std::constructible_from<T, Args...>) T* addShape(Args&&... args) {
			return m_rendererNode->constructShape<T>(args...);
		}
		void removeShape(Shape* shape) { m_rendererNode->removeShape(shape); }

		void addRendererNode(graphics::FramegraphContext& context);
		void setWindowSize(uint32_t windowWidth, uint32_t windowHeight);

		FontLibrary& fontLibrary() { return m_fontLibrary; }
		Control* rootControl() { return &m_rootControl; }

		uint32_t monitorDPIX() const { return m_windowInterface->contentScaleDPIX(); }
		uint32_t monitorDPIY() const { return m_windowInterface->contentScaleDPIY(); }

		void acquireInputFocus(Control* newInputFocusControl, const std::vector<uint32_t>& keyCodes,
							   windowing::KeyModifierFlags modifierMask, windowing::KeyStateFlags stateMask);
		void releaseInputFocus();
		Control* inputFocusControl() { return m_inputFocusControl; }

		void recalculateLayerIndices();
		void setLayerScissor(uint32_t layerIndex, VkRect2D scissorRect);
		VkRect2D layerScissor(uint32_t layerIndex);

		void invokeMouseHover(const Vector2& mousePos);
		void invokeMouseButton(uint32_t buttonID);
		void invokeKey(uint32_t keyID, windowing::KeyModifierFlags modifierFlags, windowing::KeyState stateFlags);
		void invokeCharacter(uint32_t codepoint);

	  private:
		windowing::WindowInterface* m_windowInterface;

		UIRendererNode* m_rendererNode;
		FontLibrary m_fontLibrary;
		Control m_rootControl;

		Control* m_inputFocusControl = nullptr;

		std::vector<uint32_t> m_inputFocusKeyCodes;
		windowing::KeyModifierFlags m_inputFocusModifierMask;
		windowing::KeyStateFlags m_inputFocusStateMask;

		std::vector<VkRect2D> m_layerScissors;
	};

} // namespace vanadium::ui
