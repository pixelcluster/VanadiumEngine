#version 450 core
//editor config
//! #extension GL_KHR_vulkan_glsl : require

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 inTexCoord;
layout(location = 1) in vec3 sphereNormal;

layout(set = 1, binding = 0) uniform sampler2D tex;

void main() {
	outColor = vec4(texture(tex, inTexCoord).rgb * max(dot(sphereNormal, vec3(0.0f, 1.0f, 0.0)), 0.01), 1.0f);
}