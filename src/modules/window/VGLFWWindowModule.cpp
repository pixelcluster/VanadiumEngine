#include <modules/window/VGLFWWindowModule.hpp>
#include <modules/gpu/helper/ErrorHelper.hpp>

VGLFWWindowModule::VGLFWWindowModule(uint32_t width, uint32_t height, const char* windowTitle) {
	if (!m_glfwInitialized) {
		glfwInit();
		m_glfwInitialized = true;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	m_window = glfwCreateWindow(static_cast<int>(width), static_cast<int>(height), windowTitle, nullptr, nullptr);

	m_width = width;
	m_height = height;
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
}

void VGLFWWindowModule::onDeactivate(VEngine& engine) {}

void VGLFWWindowModule::onDestroy(VEngine& engine) { glfwDestroyWindow(m_window); }

VkSurfaceKHR VGLFWWindowModule::createWindowSurface(VkInstance instance) {
	VkSurfaceKHR surface;
	verifyResult(glfwCreateWindowSurface(instance, m_window, nullptr, &surface));
	return surface;
}
