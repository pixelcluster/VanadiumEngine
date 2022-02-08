#pragma once

#include <VEngine.hpp>
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

enum class VKeyState {
	Released, Pressed, Held
};

class VWindowModule : public VModule {
  public:
	virtual VkSurfaceKHR createWindowSurface(VkInstance instance) = 0;

	virtual VKeyState keyCharState(char key) const = 0;
	virtual VKeyState mouseKeyState(unsigned int key) const = 0;

	float mouseX() const { return m_mouseX; }
	float mouseY() const { return m_mouseY; }

	uint32_t width() const { return m_width; }
	uint32_t height() const { return m_height; }

	bool isFocused() const { return m_isFocused; }
	bool wasResized() const { return m_wasResized; }

	void waitForEvents() { m_waitFlag = true; }

	double deltaTime() { return m_time - m_lastTime; }

  protected:
	uint32_t m_width, m_height;
	double m_mouseX, m_mouseY;

	bool m_isFocused = false, m_wasResized = false;
	bool m_waitFlag = false;

	double m_time = 0.0;
	double m_lastTime = 0.0;
	// TODO: add input interface
};