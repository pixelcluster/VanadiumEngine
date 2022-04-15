#version 460 core

struct RectData {
    vec2 position;
    vec2 size;
    vec4 color;
};

layout(set = 0, binding = 0) buffer Data {
    RectData data[];
};

const vec2 positions[] = {
    vec2(0.0f, 0.0f),
    vec2(0.0f, 1.0f),
    vec2(1.0f, 0.0f),
    vec2(1.0f, 1.0f)
};

const uint indices[] = {
    0, 3, 2, 1, 3, 0
};

layout(location = 0) out vec4 outColor;

void main() {
    gl_Position = vec4(positions[indices[gl_VertexIndex]] * data[gl_InstanceIndex].size + data[gl_InstanceIndex].position, 0.5f, 1.0f);
    gl_Position *= 2.0f;
    gl_Position -= 1.0f;
    outColor = data[gl_InstanceIndex].color;
}