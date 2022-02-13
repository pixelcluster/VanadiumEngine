#pragma once

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

struct PlanetObject {
	VkDescriptorSet textureDescriptorSet;

	size_t vertexOffset;
	size_t indexOffset;
	size_t instanceOffset;
};