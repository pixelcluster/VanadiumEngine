#include <Log.hpp>
#include <windowing/WindowInterface.hpp>

namespace vanadium::windowing {

	void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
		WindowInterface* interface = reinterpret_cast<WindowInterface*>(glfwGetWindowUserPointer(window));
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

	WindowInterface::WindowInterface(bool createFullscreen, uint32_t width, uint32_t height, const char* name) {
		if (!glfwWindowCount) {
			assertFatal(!glfwInit(), "GLFW initialization failed!\n");
		}

		GLFWmonitor* monitor = createFullscreen ? glfwGetPrimaryMonitor() : nullptr;
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		m_window = glfwCreateWindow(static_cast<int>(width), static_cast<int>(height), name, monitor, nullptr);

		glfwSetWindowUserPointer(m_window, this);
		glfwSetKeyCallback(m_window, keyCallback);

		assertFatal(m_window, "Couldn't create window!\n");

		++glfwWindowCount;
	}

	WindowInterface::~WindowInterface() {
		glfwDestroyWindow(m_window);
		if (!(--glfwWindowCount)) {
			glfwTerminate();
		}
	}

	void WindowInterface::pollEvents() { glfwPollEvents(); }

	void WindowInterface::addKeyListener(uint32_t keyCode, KeyModifierFlags modifierMask, KeyStateFlags stateMask,
										 const KeyListenerParams& params) {
		m_keyListeners.insert(
			{ KeyListenerData{ .keyCode = keyCode, .modifierMask = modifierMask, .keyStateMask = stateMask }, params });
	}

	void WindowInterface::removeKeyListener(uint32_t keyCode, KeyModifierFlags modifierMask, KeyStateFlags stateMask) {
		auto iterator = m_keyListeners.find(
			KeyListenerData{ .keyCode = keyCode, .modifierMask = modifierMask, .keyStateMask = stateMask });
		if(iterator != m_keyListeners.end()) {
			m_keyListeners.erase(iterator);
		}
	}

	void WindowInterface::addSizeListener(const SizeListenerParams& params) {
		m_sizeListeners.push_back(params);
	}

	void WindowInterface::removeSizeListener(const SizeListenerParams& params) {
		auto iterator = std::find(m_sizeListeners.begin(), m_sizeListeners.end(), params);
		if(iterator != m_sizeListeners.end()) {
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
} // namespace vanadium::windowing