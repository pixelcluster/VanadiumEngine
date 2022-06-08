#version 460 core


layout(location = 0) in vec2 shadowPeakPos;
layout(location = 2) in float maxOpacity;
layout(location = 3) in vec2 pos;

layout(location = 0) out vec4 color;

float sstep(float x, float e0, float e1) {
    float t = clamp((x - e0) / (e1 - e0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

void main() {
    color = vec4(sstep(pos.x, 0, shadowPeakPos.x) * sstep(pos.y, 0, shadowPeakPos.y) * 
            (1 - sstep(pos.x, 1 - shadowPeakPos.x, 1)) * (1 - sstep(pos.y, 1 - shadowPeakPos.y, 1)) * maxOpacity);
    color.rgb = vec3(0.0);
}