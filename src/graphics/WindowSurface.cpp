#define GLFW_INCLUDE_VULKAN
#define VK_NO_PROTOTYPES
#include <graphics/WindowSurface.hpp>
#include <graphics/helper/ErrorHelper.hpp>
#include <volk.h>

using namespace vanadium::windowing;

namespace vanadium::graphics
{
    void sizeChangeCallback(uint32_t width, uint32_t height, void* userData) {
        WindowSurface* surface = reinterpret_cast<WindowSurface*>(userData);
        surface->setSuggestedSize(width, height);
    }

    WindowSurface::WindowSurface(WindowInterface& interface) : m_interface(interface) {
        interface.windowSize(m_suggestedWidth, m_suggestedHeight);
        interface.addSizeListener({
            .eventCallback = sizeChangeCallback,
            .listenerDestroyCallback = emptyListenerDestroyCallback,
            .userData = this
        });
    }

    void WindowSurface::create(VkInstance instance)  {
        verifyResult(glfwCreateWindowSurface(instance, m_interface.internalHandle(), nullptr, &m_surface));
    }

    bool WindowSurface::supportsPresent(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex) {
        VkBool32 presentSupport;
        verifyResult(vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueFamilyIndex, m_surface, &presentSupport));
        return presentSupport;
    }

    void WindowSurface::createSwapchain(VkPhysicalDevice physicalDevice, VkDevice device) {
        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        verifyResult(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, m_surface, &surfaceCapabilities));

        if(surfaceCapabilities.maxImageExtent.width == 0 || surfaceCapabilities.maxImageExtent.height == 0) {
            m_canRender = false;
            return;
        }

        uint32_t width = std::clamp(m_suggestedWidth, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.height);
        uint32_t height = std::clamp(m_suggestedHeight, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);

        uint32_t imageCount = std::max(3, surfaceCapabilities.minImageCount);
        if(surfaceCapabilities.maxImageCount) {
            imageCount = std::min(imageCount, surfaceCapabilities.maxImageCount); nhjet4
        }
    }

    void WindowSurface::setSuggestedSize(uint32_t width, uint32_t height) {
        m_suggestedWidth = width;
        m_suggestedHeight = height;
        m_swapchainDirtyFlag = true;
    } 
} // namespace vanadium::graphics
