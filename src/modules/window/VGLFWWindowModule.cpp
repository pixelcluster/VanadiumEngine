#include <modules/window/VGLFWWindowModule.hpp>
#include <modules/gpu/helper/ErrorHelper.hpp>

#include <cstring>

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (key >= 32 && key <= 96) {
		VKeyState state;
		switch (action) {
			case GLFW_PRESS:
				state = VKeyState::Pressed;
				break;
			case GLFW_REPEAT:
				state = VKeyState::Held;
				break;
			default:
				state = VKeyState::Released;
				break;
		}
		reinterpret_cast<VGLFWWindowModule*>(glfwGetWindowUserPointer(window))->setKeyState(key, state);
	}
}

void mouseKeyCallback(GLFWwindow* window, int key, int action, int mods) {
	VKeyState state;
	switch (action) {
		case GLFW_PRESS:
			state = VKeyState::Pressed;
			break;
		case GLFW_REPEAT:
			state = VKeyState::Held;
			break;
		default:
			state = VKeyState::Released;
			break;
	}
	reinterpret_cast<VGLFWWindowModule*>(glfwGetWindowUserPointer(window))->setMouseKeyState(key, state);
}

VGLFWWindowModule::VGLFWWindowModule(uint32_t width, uint32_t height, const char* windowTitle) {
	if (!m_glfwInitialized) {
		glfwInit();
		m_glfwInitialized = true;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	m_window = glfwCreateWindow(static_cast<int>(width), static_cast<int>(height), windowTitle, nullptr, nullptr);

	glfwSetWindowUserPointer(m_window, this);
	glfwSetKeyCallback(m_window, keyCallback);
	glfwSetMouseButtonCallback(m_window, mouseKeyCallback);

	m_width = width;
	m_height = height;

	memset(m_keyStates, 0, INT8_MAX * sizeof(VKeyState));
	memset(m_mouseKeys, 0, (GLFW_MOUSE_BUTTON_LAST + 1) * sizeof(VKeyState));
}

void VGLFWWindowModule::onCreate(VEngine& engine) {}

void VGLFWWindowModule::onActivate(VEngine& engine) {}

void VGLFWWindowModule::onExecute(VEngine& engine) { 
	m_wasResized = false;
	if (m_waitFlag) {
		glfwWaitEvents();
		m_waitFlag = false;
	} else {
		glfwPollEvents();
	}
	if (glfwWindowShouldClose(m_window)) {
		engine.destroyModule(this);
	}
	int newWidth, newHeight;
	glfwGetFramebufferSize(m_window, &newWidth, &newHeight);

	if (static_cast<uint32_t>(newWidth) != m_width) {
		m_wasResized = true;
		m_width = static_cast<uint32_t>(newWidth);
	}
	if (static_cast<uint32_t>(newHeight) != m_height) {
		m_wasResized = true;
		m_height = static_cast<uint32_t>(newHeight);
	}

	m_lastTime = m_time;
	m_time = glfwGetTime();

	double mouseX, mouseY;
	glfwGetCursorPos(m_window, &mouseX, &mouseY);
	m_mouseX = mouseX;
	m_mouseY = mouseY;

	if (m_lastTime == 0.0f)
		m_lastTime = m_time;
}

void VGLFWWindowModule::onDeactivate(VEngine& engine) {}

void VGLFWWindowModule::onDestroy(VEngine& engine) { glfwDestroyWindow(m_window); }

VkSurfaceKHR VGLFWWindowModule::createWindowSurface(VkInstance instance) {
	VkSurfaceKHR surface;
	verifyResult(glfwCreateWindowSurface(instance, m_window, nullptr, &surface));
	return surface;
}

VKeyState VGLFWWindowModule::keyCharState(char key) const { return m_keyStates[key]; }

VKeyState VGLFWWindowModule::mouseKeyState(unsigned int key) const { return m_mouseKeys[key]; }
