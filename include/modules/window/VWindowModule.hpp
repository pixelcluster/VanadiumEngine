#pragma once

#include <VEngine.hpp>
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

class VWindowModule : public VModule {
  public:
	  virtual VkSurfaceKHR createWindowSurface(VkInstance instance) = 0;

	  uint32_t width() const { return m_width; }
	  uint32_t height() const { return m_height; }

	  bool isFocused() const { return m_isFocused; }
	  bool wasResized() const { return m_wasResized; }

	  void waitForEvents() { m_waitFlag = true; }
  protected:
	uint32_t m_width, m_height;
	bool m_isFocused = false, m_wasResized = false;
	  bool m_waitFlag = false;
	//TODO: add input interface
};