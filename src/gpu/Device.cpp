#include <modules/gpu/Device.hpp>
#include <modules/gpu/helper/EnumerationHelper.hpp>
#include <modules/gpu/helper/ErrorHelper.hpp>
#include <modules/gpu/QueueFamilyLayout.hpp>
#include <volk.h>

namespace gpu {

	Device::Device(VkInstance instance, VkSurfaceKHR surface, const std::vector<QueueFamilyLayout> queueFamilies,
				   const std::vector<std::string_view>& extensions, const VkPhysicalDeviceFeatures2& features) {
		std::vector<VkPhysicalDevice> devices = enumerate(instance, vkEnumeratePhysicalDevices);

		size_t deviceIndex = -1U;
		bool wasDeviceCPU = false;
		float numQueueMatches = 0.0f;

		std::vector<uint32_t> layoutQueueFamilyIndices = std::vector<uint32_t>(queueFamilies.size(), -1U);
		std::vector<VkPhysicalDeviceProperties> properties = std::vector<VkPhysicalDeviceProperties>(devices.size());

		uint32_t currentDeviceIndex = 0;
		for (auto& device : devices) {
			vkGetPhysicalDeviceProperties(device, &properties[currentDeviceIndex]);

			std::vector<VkQueueFamilyProperties> queueFamilyProperties;
			uint32_t queueFamilyPropertyCount;
			vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyPropertyCount, nullptr);
			queueFamilyProperties.resize(queueFamilyPropertyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyPropertyCount, queueFamilyProperties.data());

			std::vector<VkExtensionProperties> extensionProperties =
				enumerate<VkPhysicalDevice, VkExtensionProperties, const char*>(device, nullptr,
																				vkEnumerateDeviceExtensionProperties);

			bool supportsExtensions = true;
			for (auto& name : extensions) {
				bool supportsExtension = false;
				for (auto& extension : extensionProperties) {
					supportsExtension |= name == extension.extensionName;
				}
				supportsExtensions &= supportsExtension;
				if (!supportsExtension)
					break;
			}
			if (!supportsExtensions)
				continue;

			uint32_t queueFamilyIndex = 0;
			float queueLayoutMatchCount = 0.0f;

			bool foundAnyPresentMatch = false;
			bool requiredAnyPresent = false;

			for (auto& queueFamily : queueFamilyProperties) {
				size_t layoutIndex = 0;
				for (auto& layout : queueFamilies) {
					VkBool32 supportsPresenting;
					verifyResult(
						vkGetPhysicalDeviceSurfaceSupportKHR(device, queueFamilyIndex, surface, &supportsPresenting));

					if ((supportsPresenting || !layout.shouldSupportPresent) &&
						(layout.queueCapabilityFlags & queueFamily.queueFlags) == layout.queueCapabilityFlags) {
						if (queueFamily.queueCount >= layout.queueCount) {
							queueLayoutMatchCount += 1.0f;
							layoutQueueFamilyIndices[layoutIndex] = queueFamilyIndex;
						} else {
							queueLayoutMatchCount += 0.5f;
							if (layoutQueueFamilyIndices[layoutIndex] == -1U) {
								layoutQueueFamilyIndices[layoutIndex] = queueFamilyIndex;
							}
						}
					}
				}

				++queueFamilyIndex;
			}

			if (numQueueMatches > queueLayoutMatchCount) {
			}

			++currentDeviceIndex;
		}
	}

} // namespace gpu