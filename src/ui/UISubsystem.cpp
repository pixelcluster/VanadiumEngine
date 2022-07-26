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
#include <ui/UISubsystem.hpp>

namespace vanadium::ui {

	void windowSizeListener(uint32_t width, uint32_t height, void* userData) {
		UISubsystem* subsystem = std::launder(reinterpret_cast<UISubsystem*>(userData));
		subsystem->setWindowSize(width, height);
	}

	void mouseButtonListener(uint32_t keyCode, uint32_t modifiers, windowing::KeyState state, void* userData) {
		UISubsystem* subsystem = std::launder(reinterpret_cast<UISubsystem*>(userData));
		subsystem->invokeMouseButton(keyCode);
	}

	void mouseListener(const Vector2& pos, void* userData) {
		UISubsystem* subsystem = std::launder(reinterpret_cast<UISubsystem*>(userData));
		subsystem->invokeMouseHover(pos);
	}

	void keyListener(uint32_t keyCode, uint32_t modifiers, windowing::KeyState state, void* userData) {
		UISubsystem* subsystem = std::launder(reinterpret_cast<UISubsystem*>(userData));
		subsystem->invokeKey(keyCode, modifiers, state);
	}

	void charListener(uint32_t codepoint, void* userData) {
		UISubsystem* subsystem = std::launder(reinterpret_cast<UISubsystem*>(userData));
		subsystem->invokeCharacter(codepoint);
	}

	UISubsystem::UISubsystem(windowing::WindowInterface* windowInterface, const graphics::RenderContext& context,
							 const std::string_view& fontLibraryFile, const Vector4& clearValue)
		: m_windowInterface(windowInterface), m_fontLibrary(fontLibraryFile),
		  m_rootControl(this, nullptr, ControlPosition(PositionOffsetType::TopLeft, Vector2(0.0f, 0.0f)),
						Vector2(0.0f, 0.0f), createStyle<Style>(), createLayout<Layout>(),
						createFunctionality<Style, Functionality>()) {
		m_rendererNode = new UIRendererNode(this, context, clearValue);

		windowInterface->addSizeListener({ .eventCallback = windowSizeListener,
										   .listenerDestroyCallback = windowing::emptyListenerDestroyCallback,
										   .userData = this });
		windowInterface->addMouseKeyListener(GLFW_MOUSE_BUTTON_LEFT, static_cast<windowing::KeyModifierFlags>(~0U),
											 static_cast<windowing::KeyStateFlags>(windowing::KeyState::Pressed),
											 { .eventCallback = mouseButtonListener,
											   .listenerDestroyCallback = windowing::emptyListenerDestroyCallback,
											   .userData = this });
		windowInterface->addMouseMoveListener({ .eventCallback = mouseListener,
												.listenerDestroyCallback = windowing::emptyListenerDestroyCallback,
												.userData = this });
		windowInterface->addCharacterListener({ .eventCallback = charListener,
												.listenerDestroyCallback = windowing::emptyListenerDestroyCallback,
												.userData = this });
	}

	void UISubsystem::addRendererNode(graphics::FramegraphContext& context) {
		context.insertExistingNode(nullptr, m_rendererNode);
		m_rendererNode->create(&context);
	}

	void UISubsystem::setWindowSize(uint32_t windowWidth, uint32_t windowHeight) {
		m_rootControl.setSize(Vector2(windowWidth, windowHeight));
	}

	void UISubsystem::acquireInputFocus(Control* newInputFocusControl, const std::vector<uint32_t>& keyCodes,
										windowing::KeyModifierFlags modifierMask, windowing::KeyStateFlags stateMask) {
		if (m_inputFocusControl) {
			for (auto& keyCode : m_inputFocusKeyCodes) {
				m_windowInterface->removeKeyListener(
					keyCode, m_inputFocusModifierMask, m_inputFocusStateMask,
					{ .eventCallback = keyListener,
					  .listenerDestroyCallback = windowing::emptyListenerDestroyCallback,
					  .userData = this });
			}
			m_inputFocusControl->releaseInputFocus(this);
		}
		m_inputFocusControl = newInputFocusControl;
		m_inputFocusKeyCodes = keyCodes;
		m_inputFocusModifierMask = modifierMask;
		m_inputFocusStateMask = stateMask;
		for (auto& keyCode : keyCodes) {
			m_windowInterface->addKeyListener(keyCode, modifierMask, stateMask,
											  { .eventCallback = keyListener,
												.listenerDestroyCallback = windowing::emptyListenerDestroyCallback,
												.userData = this });
		}
	}

	void UISubsystem::releaseInputFocus() {
		if (m_inputFocusControl) {
			m_inputFocusControl->releaseInputFocus(this);
		}
		m_inputFocusControl = nullptr;
		for (auto& keyCode : m_inputFocusKeyCodes) {
			m_windowInterface->removeKeyListener(keyCode, m_inputFocusModifierMask, m_inputFocusStateMask,
												 { .eventCallback = keyListener,
												   .listenerDestroyCallback = windowing::emptyListenerDestroyCallback,
												   .userData = this });
		}
	}

	void UISubsystem::recalculateLayerIndices() {
		uint32_t layerIndex = 0;
		m_rootControl.internalRecalculateLayerIndex(layerIndex);
	}

	void UISubsystem::setLayerScissor(uint32_t layerIndex, VkRect2D scissorRect) {
		if(m_layerScissors.size() <= layerIndex) {
			m_layerScissors.resize(layerIndex + 1, {});
		}
		m_layerScissors[layerIndex] = scissorRect;
	}

	VkRect2D UISubsystem::layerScissor(uint32_t layerIndex) {
		if(m_layerScissors.size() <= layerIndex) {
			m_layerScissors.resize(layerIndex + 1, {});
		}
		return m_layerScissors[layerIndex];
	}

	void UISubsystem::invokeMouseHover(const Vector2& mousePos) { m_rootControl.invokeHoverHandler(this, mousePos); }

	void UISubsystem::invokeMouseButton(uint32_t buttonID) {
		Vector2 mousePos = m_windowInterface->mousePos();
		m_rootControl.invokeMouseButtonHandler(this, mousePos, buttonID);
	}

	void UISubsystem::invokeKey(uint32_t keyID, windowing::KeyModifierFlags modifierFlags,
								windowing::KeyState stateFlags) {
		if (m_inputFocusControl)
			m_inputFocusControl->invokeKeyInputHandler(this, keyID, modifierFlags, stateFlags);
	}

	void UISubsystem::invokeCharacter(uint32_t codepoint) {
		if (m_inputFocusControl)
			m_inputFocusControl->invokeCharInputHandler(this, codepoint);
	}

} // namespace vanadium::ui
