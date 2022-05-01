#version 460 core

struct RectData {
    vec3 position;
    vec4 color;
    vec2 size;
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
    0, 2, 2, 3, 3, 1, 1, 0
};

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PushConstantData {
    vec2 targetDimensions;
};

void main() {
    uint instanceIndex = uint(floor(gl_VertexIndex / 8.0f));
    gl_Position = vec4(positions[indices[gl_VertexIndex % 8]] * data[instanceIndex].size / targetDimensions + data[instanceIndex].position.xy / targetDimensions, data[instanceIndex].position.z, 1.0f);
    gl_Position *= 2.0f;
    gl_Position -= 1.0f;
    outColor = data[instanceIndex].color;
}