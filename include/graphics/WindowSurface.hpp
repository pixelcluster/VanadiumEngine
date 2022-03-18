#include <windowing/WindowInterface.hpp>

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

namespace vanadium::graphics {
	class WindowSurface {
	  public:
		WindowSurface(windowing::WindowInterface& interface);
		~WindowSurface();

		void create(VkInstance instance);

		bool supportsPresent(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex);
		void createSwapchain(VkPhysicalDevice physicalDevice, VkDevice device);

		void destroy(VkDevice device, VkInstance instance);

		bool surfaceDirtyFlag() const { return m_surfaceDirtyFlag; }
		bool swapchainDirtyFlag() const { return m_swapchainDirtyFlag; }
		bool canRender() const { return m_canRender; }

        void setSuggestedSize(uint32_t suggestedWidth, uint32_t suggestedHeight);

        //returns whether the swapchain data was changed
        bool refreshSwapchain();

	  private:
		windowing::WindowInterface& m_interface;

		bool m_surfaceDirtyFlag = true;
		bool m_swapchainDirtyFlag = true;
		bool m_canRender = false;

        uint32_t m_suggestedWidth = 0, m_suggestedHeight = 0;

		VkSurfaceKHR m_surface;
		VkSwapchainKHR m_swapchain;
	};

} // namespace vanadium::graphics
