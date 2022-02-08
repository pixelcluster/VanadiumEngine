#pragma once

#include <VEngine.hpp>
#include <modules/window/VWindowModule.hpp>
#define GLFW_INCLUDE_VULKAN // only after vulkan.h has already been included, don't want to leak prototypes
#include <GLFW/glfw3.h>

class VGLFWWindowModule : public VWindowModule {
  public:
	VGLFWWindowModule(uint32_t width, uint32_t height, const char* windowTitle);

	virtual void onCreate(VEngine& engine) override;
	virtual void onActivate(VEngine& engine) override;
	virtual void onExecute(VEngine& engine) override;
	virtual void onDeactivate(VEngine& engine) override;
	virtual void onDestroy(VEngine& engine) override;
	virtual VkSurfaceKHR createWindowSurface(VkInstance instance) override;
	virtual VKeyState keyCharState(char key) const override;
	virtual VKeyState mouseKeyState(unsigned int key) const override;

	void setKeyState(char key, VKeyState state) { m_keyStates[key] = state; }
	void setMouseKeyState(unsigned int keyIndex, VKeyState state) { m_mouseKeys[keyIndex] = state; }

  private:
	GLFWwindow* m_window;

	VKeyState m_keyStates[INT8_MAX];
	VKeyState m_mouseKeys[GLFW_MOUSE_BUTTON_LAST + 1];

	inline static bool m_glfwInitialized = false;
};