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

	void errorCallback(int code, const char* desc) {
		logError("GLFW Error: %s%s", desc, "\n");
	}

	WindowInterface::WindowInterface(const std::optional<WindowingSettingOverride>& override, const char* name) {
		WindowingSettingOverride value =
			override.value_or(WindowingSettingOverride{ .width = 640, .height = 480, .createFullScreen = false });
		
		glfwSetErrorCallback(errorCallback);
		assertFatal(glfwInit(), "GLFW initialization failed!\n");

		GLFWmonitor* monitor = value.createFullScreen ? glfwGetPrimaryMonitor() : nullptr;
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		m_window =
			glfwCreateWindow(static_cast<int>(value.width), static_cast<int>(value.height), name, monitor, nullptr);

		glfwSetWindowUserPointer(m_window, this);
		glfwSetKeyCallback(m_window, keyCallback);

		assertFatal(m_window, "Couldn't create window!\n");

		++m_glfwWindowCount;
	}

	WindowInterface::~WindowInterface() {
		glfwDestroyWindow(m_window);
		if (!(--m_glfwWindowCount)) {
			glfwTerminate();
		}
	}

	void WindowInterface::pollEvents() { glfwPollEvents(); }

	void WindowInterface::waitEvents() { glfwWaitEvents(); }

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

	void WindowInterface::addSizeListener(const SizeListenerParams& params) { m_sizeListeners.push_back(params); }

	void WindowInterface::removeSizeListener(const SizeListenerParams& params) {
		auto iterator = std::find(m_sizeListeners.begin(), m_sizeListeners.end(), params);
		if (iterator != m_sizeListeners.end()) {
			m_sizeListeners.erase(iterator);
		}
	}

	void WindowInterface::invokeKeyListeners(uint32_t keyCode, KeyModifierFlags modifiers, KeyState state) {
		for (auto& listener : m_keyListeners) {
			if (listener.first.keyCode == keyCode && (listener.first.modifierMask & modifiers) &&
				(listener.first.keyStateMask & state)) {
				listener.second.eventCallback(keyCode, modifiers, state, listener.second.userData);
			}
		}
	}

	void WindowInterface::windowSize(uint32_t& width, uint32_t& height) {
		int glfwWidth, glfwHeight;
		glfwGetFramebufferSize(m_window, &glfwWidth, &glfwHeight);
		width = static_cast<uint32_t>(glfwWidth);
		height = static_cast<uint32_t>(glfwHeight);
	}

	bool WindowInterface::shouldClose() {
		return glfwWindowShouldClose(m_window);
	}
} // namespace vanadium::windowing