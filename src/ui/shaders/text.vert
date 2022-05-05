#version 460 core

struct RectData {
    vec3 position;
    vec4 color;
    vec2 size;
    vec2 uvOffset;
    vec2 uvSize;
    vec2 cosSinRotation;
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

layout(push_constant) uniform PushConstantData {
    vec2 targetDimensions;
};

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec2 uvPos;

void main() {
    uint instanceIndex = uint(floor(gl_VertexIndex / 6.0f));

    vec2 pos = positions[indices[gl_VertexIndex % 6]];
    uvPos = pos * data[instanceIndex].uvSize + data[instanceIndex].uvOffset;

    mat2 rotation = mat2(data[instanceIndex].cosSinRotation.x, data[instanceIndex].cosSinRotation.y, 
                        -data[instanceIndex].cosSinRotation.y, data[instanceIndex].cosSinRotation.x);

    pos = rotation * (pos * data[instanceIndex].size);

    gl_Position = vec4(pos / targetDimensions + data[instanceIndex].position.xy / targetDimensions, data[instanceIndex].position.z, 1.0f);
    gl_Position *= 2.0f;
    gl_Position -= 1.0f;
    
    outColor = data[instanceIndex].color;
}
