#version 450 core
//editor config
//! #extension GL_KHR_vulkan_glsl : require

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 inTexCoord;

layout(set = 1, binding = 0) uniform sampler2D tex;

void main() {
	outColor = vec4(texture(tex, inTexCoord).rgb, 1.0f);
}