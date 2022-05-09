#version 460 core

layout(set = 0, binding = 1) uniform sampler2D fontAtlasSampler;

layout(location = 0) in vec4 color;
layout(location = 1) in vec2 uvPos;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(texture(fontAtlasSampler, uvPos).r * color);
}