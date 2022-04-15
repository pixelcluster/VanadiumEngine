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
			interface->invokeKeyListeners(static_cast<uint32_t>(key), static_cast<KeyModifierFlags>(mods), state);
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
			interface->invokeMouseKeyListeners(static_cast<uint32_t>(button), static_cast<KeyModifierFlags>(mods),
											   state);
	}

	void sizeCallback(GLFWwindow* window, int width, int height) {
		WindowInterface* interface = std::launder(reinterpret_cast<WindowInterface*>(glfwGetWindowUserPointer(window)));
		interface->invokeSizeListeners(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
	}

	void errorCallback(int code, const char* desc) { logError("GLFW Error: %s%s", desc, "\n"); }

	WindowInterface::WindowInterface(const std::optional<WindowingSettingOverride>& override, const char* name) {
		WindowingSettingOverride value =
			override.value_or(WindowingSettingOverride{ .width = 640, .height = 480, .createFullScreen = false });

		glfwSetErrorCallback(errorCallback);
		assertFatal(glfwInit(), "GLFW initialization failed!\n");

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

		assertFatal(m_window, "Couldn't create window!\n");

		++m_glfwWindowCount;
	}

	WindowInterface::~WindowInterface() {
		for (auto& listener : m_keyListeners) {
			listener.second.listenerDestroyCallback(listener.second.userData);
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

	void WindowInterface::addKeyListener(uint32_t keyCode, KeyModifierFlags modifierMask, KeyStateFlags stateMask,
										 const KeyListenerParams& params) {
		m_keyListeners.insert(
			{ KeyListenerData{ .keyCode = keyCode, .modifierMask = modifierMask, .keyStateMask = stateMask }, params });
	}

	void WindowInterface::removeKeyListener(uint32_t keyCode, KeyModifierFlags modifierMask, KeyStateFlags stateMask) {
		auto iterator = m_keyListeners.find(
			KeyListenerData{ .keyCode = keyCode, .modifierMask = modifierMask, .keyStateMask = stateMask });
		if (iterator != m_keyListeners.end()) {
			m_keyListeners.erase(iterator);
		}
	}

	void WindowInterface::addMouseKeyListener(uint32_t keyCode, KeyModifierFlags modifierMask, KeyStateFlags stateMask,
											  const KeyListenerParams& params) {
		m_mouseKeyListeners.insert(
			{ KeyListenerData{ .keyCode = keyCode, .modifierMask = modifierMask, .keyStateMask = stateMask }, params });
	}

	void WindowInterface::removeMouseKeyListener(uint32_t keyCode, KeyModifierFlags modifierMask,
												 KeyStateFlags stateMask) {
		auto iterator = m_mouseKeyListeners.find(
			KeyListenerData{ .keyCode = keyCode, .modifierMask = modifierMask, .keyStateMask = stateMask });
		if (iterator != m_mouseKeyListeners.end()) {
			m_mouseKeyListeners.erase(iterator);
		}
	}

	void WindowInterface::addSizeListener(const SizeListenerParams& params) { m_sizeListeners.push_back(params); }

	void WindowInterface::removeSizeListener(const SizeListenerParams& params) {
		auto iterator = std::find(m_sizeListeners.begin(), m_sizeListeners.end(), params);
		if (iterator != m_sizeListeners.end()) {
			m_sizeListeners.erase(iterator);
		}
	}

	void WindowInterface::invokeKeyListeners(uint32_t keyCode, KeyModifierFlags modifiers, KeyState state) {
		for (auto& listener : m_keyListeners) {
			if (listener.first.keyCode == keyCode &&
				((listener.first.modifierMask & modifiers) || listener.first.modifierMask == 0) &&
				((listener.first.keyStateMask & state) || listener.first.keyStateMask == 0)) {
				listener.second.eventCallback(keyCode, modifiers, state, listener.second.userData);
			}
		}
	}

	void WindowInterface::invokeMouseKeyListeners(uint32_t keyCode, KeyModifierFlags modifiers, KeyState state) {
		for (auto& listener : m_mouseKeyListeners) {
			if (listener.first.keyCode == keyCode &&
				((listener.first.modifierMask & modifiers) || listener.first.modifierMask == 0) &&
				((listener.first.keyStateMask & state) || listener.first.keyStateMask == 0)) {
				listener.second.eventCallback(keyCode, modifiers, state, listener.second.userData);
			}
		}
	}

	void WindowInterface::invokeSizeListeners(uint32_t newWidth, uint32_t newHeight) {
		for (auto& listener : m_sizeListeners) {
			listener.eventCallback(newWidth, newHeight, listener.userData);
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