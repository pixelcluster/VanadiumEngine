#version 460 core

layout(set = 0, binding = 0) uniform Data {
    vec2 position;
    vec2 size;
    vec4 color;
};

const vec2 positions[] = {
    vec2(0.0f, 0.0f),
    vec2(0.0f, 1.0f),
    vec2(1.0f, 0.0f),
    vec2(1.0f, 1.0f)
};

layout(location = 0) out vec4 outColor;

void main() {
    gl_Position = vec4(positions[gl_VertexIndex] * size + position, 0.0f, 1.0f);
    outColor = color;
}