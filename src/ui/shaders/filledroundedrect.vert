#version 460 core

struct RectData {
    vec2 position;
    vec2 size;
    vec4 color;
    vec2 cosSinRotation;
    float edgeSize;
};

layout(std430, set = 0, binding = 0) buffer Data {
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
layout(location = 1) out float edgeSize;
layout(location = 2) out vec2 position;
layout(location = 3) out float shapeAspectRatio;

layout(push_constant) uniform PushConstantData {
    vec2 targetDimensions;
    uint instanceOffset;
};

void main() {
    position = positions[indices[gl_VertexIndex % 6]];

    uint instanceIndex = uint(floor(gl_VertexIndex / 6.0f)) + instanceOffset;

    mat2 rotation = mat2(data[instanceIndex].cosSinRotation.x, data[instanceIndex].cosSinRotation.y, 
                        -data[instanceIndex].cosSinRotation.y, data[instanceIndex].cosSinRotation.x);

    gl_Position = vec4((rotation * (position * data[instanceIndex].size)) / targetDimensions + data[instanceIndex].position.xy / targetDimensions, 0.5f, 1.0f);
    gl_Position *= 2.0f;
    gl_Position -= 1.0f;
    outColor = data[instanceIndex].color;
    edgeSize = data[instanceIndex].edgeSize;
    shapeAspectRatio = data[instanceIndex].size.x / data[instanceIndex].size.y;
    position.x *= shapeAspectRatio;
}
