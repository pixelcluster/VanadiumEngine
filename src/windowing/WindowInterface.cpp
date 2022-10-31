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
#include <Log.hpp>
#include <windowing/WindowInterface.hpp>

namespace vanadium::windowing {

	uint32_t WindowInterface::m_glfwWindowCount = 0;

	void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
		WindowInterface* interface = std::launder(reinterpret_cast<WindowInterface*>(glfwGetWindowUserPointer(window)));
		KeyState state;

		switch (action) {
			case GLFW_RELEASE:
				state = KeyState::Released;
				break;
			case GLFW_PRESS:
				state = KeyState::Pressed;
				break;
			case GLFW_REPEAT:
				state = KeyState::Held;
				break;
			default:
				state = KeyState::Pressed;
				break;
		}
		if (key != GLFW_KEY_UNKNOWN)
			interface->invokeKeyListeners(static_cast<uint32_t>(key), static_cast<KeyModifier>(mods), state);
	}

	void mouseKeyCallback(GLFWwindow* window, int button, int action, int mods) {
		WindowInterface* interface = std::launder(reinterpret_cast<WindowInterface*>(glfwGetWindowUserPointer(window)));
		KeyState state;

		switch (action) {
			case GLFW_RELEASE:
				state = KeyState::Released;
				break;
			case GLFW_PRESS:
				state = KeyState::Pressed;
				break;
			case GLFW_REPEAT:
				state = KeyState::Held;
				break;
			default:
				state = KeyState::Pressed;
				break;
		}
		if (button != GLFW_KEY_UNKNOWN)
			interface->invokeMouseKeyListeners(static_cast<uint32_t>(button), static_cast<KeyModifier>(mods),
											   state);
	}

	void sizeCallback(GLFWwindow* window, int width, int height) {
		WindowInterface* interface = std::launder(reinterpret_cast<WindowInterface*>(glfwGetWindowUserPointer(window)));
		interface->invokeSizeListeners(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
	}

	void mouseMoveCallback(GLFWwindow* window, double x, double y) {
		WindowInterface* interface = std::launder(reinterpret_cast<WindowInterface*>(glfwGetWindowUserPointer(window)));

		// Translate screen coordinates to pixels
		int width, height;
		glfwGetWindowSize(window, &width, &height);
		double normalizedX = x / width, normalizedY = y / width;

		uint32_t pixelWidth, pixelHeight;
		interface->windowSize(pixelWidth, pixelHeight);
		interface->invokeMouseMoveListeners(normalizedX * pixelWidth, normalizedY * pixelHeight);
	}

	void scrollCallback(GLFWwindow* window, double x, double y) {
		WindowInterface* interface = std::launder(reinterpret_cast<WindowInterface*>(glfwGetWindowUserPointer(window)));

		// Translate screen coordinates to pixels
		int width, height;
		glfwGetWindowSize(window, &width, &height);
		double normalizedX = x / width, normalizedY = y / width;

		uint32_t pixelWidth, pixelHeight;
		interface->windowSize(pixelWidth, pixelHeight);
		interface->invokeScrollListeners(normalizedX * pixelWidth, normalizedY * pixelHeight);
	}

	void charCallback(GLFWwindow* window, uint32_t codepoint) {
		WindowInterface* interface = std::launder(reinterpret_cast<WindowInterface*>(glfwGetWindowUserPointer(window)));
		interface->invokeCharacterListeners(codepoint);
	}

	void errorCallback(int code, const char* desc) { logError(SubsystemID::RHI, "GLFW Error: {}", desc); }

	WindowInterface::WindowInterface(const std::optional<WindowingSettingOverride>& override, const char* name) {
		WindowingSettingOverride value =
			override.value_or(WindowingSettingOverride{ .width = 640, .height = 480, .createFullScreen = false });

		glfwSetErrorCallback(errorCallback);

		glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_WAYLAND);
		if (!glfwInit()) {
			glfwInitHint(GLFW_PLATFORM, GLFW_ANY_PLATFORM);
			assertFatal(glfwInit(), SubsystemID::RHI, "GLFW initialization failed!");
		}

		GLFWmonitor* monitor = value.createFullScreen ? glfwGetPrimaryMonitor() : nullptr;
		if (value.createFullScreen && (value.width == 0 || value.height == 0)) {
			const GLFWvidmode* vidmode = glfwGetVideoMode(monitor);
			value.width = vidmode->width;
			value.height = vidmode->height;
		}
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		m_window =
			glfwCreateWindow(static_cast<int>(value.width), static_cast<int>(value.height), name, monitor, nullptr);

		glfwSetWindowUserPointer(m_window, this);
		glfwSetKeyCallback(m_window, keyCallback);
		glfwSetFramebufferSizeCallback(m_window, sizeCallback);
		glfwSetMouseButtonCallback(m_window, mouseKeyCallback);
		glfwSetCursorPosCallback(m_window, mouseMoveCallback);
		glfwSetScrollCallback(m_window, scrollCallback);
		glfwSetCharCallback(m_window, charCallback);

		float contentScaleX, contentScaleY;
		glfwGetMonitorContentScale(glfwGetPrimaryMonitor(), &contentScaleX, &contentScaleY);

		m_contentScaleDPIX = platformDefaultDPI * contentScaleX;
		m_contentScaleDPIY = platformDefaultDPI * contentScaleY;

		assertFatal(m_window, SubsystemID::RHI, "Couldn't create window!");

		++m_glfwWindowCount;
	}

	WindowInterface::~WindowInterface() {
		for (auto [key, listenerGroup] : m_keyListeners) {
			for (auto& listener : listenerGroup) {
				listener.listenerDestroyCallback(listener.userData);
			}
		}
		for (auto& listener : m_sizeListeners) {
			listener.listenerDestroyCallback(listener.userData);
		}
		glfwDestroyWindow(m_window);
		if (!(--m_glfwWindowCount)) {
			glfwTerminate();
		}
	}

	void WindowInterface::pollEvents() {
		glfwPollEvents();
		float newTime = static_cast<float>(glfwGetTime());
		m_deltaTime = newTime - m_elapsedTime;
		m_elapsedTime = newTime;
	}

	void WindowInterface::waitEvents() {
		glfwWaitEvents();
		float newTime = static_cast<float>(glfwGetTime());
		m_deltaTime = newTime - m_elapsedTime;
		m_elapsedTime = newTime;
	}

	void WindowInterface::addKeyListener(uint32_t keyCode, KeyModifier modifierMask, KeyState stateMask,
										 const KeyListenerParams& params) {
		m_keyListeners[{ .keyCode = keyCode, .modifierMask = modifierMask, .keyStateMask = stateMask }].push_back(
			params);
	}

	void WindowInterface::removeKeyListener(uint32_t keyCode, KeyModifier modifierMask, KeyState stateMask,
											const KeyListenerParams& params) {
		auto iterator = m_keyListeners.find(
			KeyListenerData{ .keyCode = keyCode, .modifierMask = modifierMask, .keyStateMask = stateMask });
		if (iterator != m_keyListeners.end()) {
			auto groupIterator = std::find(iterator->second.begin(), iterator->second.end(), params);
			if (groupIterator != iterator->second.end()) {
				iterator->second.erase(groupIterator);
			}
		}
	}

	void WindowInterface::addMouseKeyListener(uint32_t keyCode, KeyModifier modifierMask, KeyState stateMask,
											  const KeyListenerParams& params) {
		m_mouseKeyListeners[{ .keyCode = keyCode, .modifierMask = modifierMask, .keyStateMask = stateMask }].push_back(
			params);
	}

	void WindowInterface::removeMouseKeyListener(uint32_t keyCode, KeyModifier modifierMask,
												 KeyState stateMask, const KeyListenerParams& params) {
		auto iterator = m_mouseKeyListeners.find(
			KeyListenerData{ .keyCode = keyCode, .modifierMask = modifierMask, .keyStateMask = stateMask });
		if (iterator != m_mouseKeyListeners.end()) {
			auto groupIterator = std::find(iterator->second.begin(), iterator->second.end(), params);
			if (groupIterator != iterator->second.end()) {
				iterator->second.erase(groupIterator);
			}
		}
	}

	void WindowInterface::addCharacterListener(const CharacterListenerParams& params) {
		m_characterListeners.push_back(params);
	}

	void WindowInterface::removeCharacterListener(const CharacterListenerParams& params) {
		auto iterator = std::find(m_characterListeners.begin(), m_characterListeners.end(), params);
		if (iterator != m_characterListeners.end()) {
			m_characterListeners.erase(iterator);
		}
	}

	void WindowInterface::addMouseMoveListener(const MouseListenerParams& params) {
		m_mouseMoveListeners.push_back(params);
	}

	void WindowInterface::removeMouseMoveListener(const MouseListenerParams& params) {
		auto iterator = std::find(m_mouseMoveListeners.begin(), m_mouseMoveListeners.end(), params);
		if (iterator != m_mouseMoveListeners.end()) {
			m_mouseMoveListeners.erase(iterator);
		}
	}

	void WindowInterface::addScrollListener(const MouseListenerParams& params) { m_scrollListeners.push_back(params); }

	void WindowInterface::removeScrollListener(const MouseListenerParams& params) {
		auto iterator = std::find(m_scrollListeners.begin(), m_scrollListeners.end(), params);
		if (iterator != m_scrollListeners.end()) {
			m_scrollListeners.erase(iterator);
		}
	}

	void WindowInterface::addSizeListener(const SizeListenerParams& params) { m_sizeListeners.push_back(params); }

	void WindowInterface::removeSizeListener(const SizeListenerParams& params) {
		auto iterator = std::find(m_sizeListeners.begin(), m_sizeListeners.end(), params);
		if (iterator != m_sizeListeners.end()) {
			m_sizeListeners.erase(iterator);
		}
	}

	void WindowInterface::invokeKeyListeners(uint32_t keyCode, KeyModifier modifiers, KeyState state) {
		for (auto [key, listenerGroup] : m_keyListeners) {
			if (key.keyCode == keyCode && ((key.modifierMask & modifiers) != KeyModifier::None || modifiers == KeyModifier::None) &&
				(key.keyStateMask & state) != KeyState::None) {
				for (auto& listener : listenerGroup) {
					listener.eventCallback(keyCode, modifiers, state, listener.userData);
				}
			}
		}
	}

	void WindowInterface::invokeMouseKeyListeners(uint32_t keyCode, KeyModifier modifiers, KeyState state) {
		for (auto [key, listenerGroup] : m_mouseKeyListeners) {
			if (key.keyCode == keyCode && ((key.modifierMask & modifiers) != KeyModifier::None || modifiers == KeyModifier::None) &&
				(key.keyStateMask & state) != KeyState::None) {
				for (auto& listener : listenerGroup) {
					listener.eventCallback(keyCode, modifiers, state, listener.userData);
				}
			}
		}
	}

	void WindowInterface::invokeCharacterListeners(uint32_t keyCode) {
		for (auto listener : m_characterListeners) {
			listener.eventCallback(keyCode, listener.userData);
		}
	}

	void WindowInterface::invokeSizeListeners(uint32_t newWidth, uint32_t newHeight) {
		for (auto& listener : m_sizeListeners) {
			listener.eventCallback(newWidth, newHeight, listener.userData);
		}
	}

	void WindowInterface::invokeMouseMoveListeners(double x, double y) {
		for (auto& listener : m_mouseMoveListeners) {
			listener.eventCallback(Vector2(x, y), listener.userData);
		}
	}

	void WindowInterface::invokeScrollListeners(double x, double y) {
		for (auto& listener : m_scrollListeners) {
			listener.eventCallback(Vector2(x, y), listener.userData);
		}
	}

	void WindowInterface::windowSize(uint32_t& width, uint32_t& height) {
		int glfwWidth, glfwHeight;
		glfwGetFramebufferSize(m_window, &glfwWidth, &glfwHeight);
		width = static_cast<uint32_t>(glfwWidth);
		height = static_cast<uint32_t>(glfwHeight);
	}

	Vector2 WindowInterface::mousePos() const {
		double x, y;
		glfwGetCursorPos(m_window, &x, &y);
		return Vector2(static_cast<float>(x), static_cast<float>(y));
	}

	bool WindowInterface::shouldClose() { return glfwWindowShouldClose(m_window); }
} // namespace vanadium::windowing
