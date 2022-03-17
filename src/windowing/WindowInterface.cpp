#include <Log.hpp>
#include <windowing/WindowInterface.hpp>

namespace vanadium::windowing {

	WindowInterface::WindowInterface(bool createFullscreen, uint32_t width, uint32_t height, const char* name) {
		if (!glfwWindowCount) {
			assertFatal(!glfwInit(), "GLFW initialization failed!\n");
		}

		GLFWmonitor* monitor = createFullscreen ? glfwGetPrimaryMonitor() : nullptr;
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		m_window = glfwCreateWindow(static_cast<int>(width), static_cast<int>(height), name, monitor, nullptr);

        assertFatal(m_window, "Couldn't create window!\n");

		++glfwWindowCount;
	}
} // namespace vanadium::windowing