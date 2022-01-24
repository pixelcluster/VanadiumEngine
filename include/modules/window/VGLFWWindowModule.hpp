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

  private:
	GLFWwindow* m_window;

	inline static bool m_glfwInitialized = false;
};