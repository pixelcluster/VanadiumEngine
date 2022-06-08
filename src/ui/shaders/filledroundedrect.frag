#version 460 core

layout(location = 0) out vec4 color;

layout(location = 0) in vec4 inColor;
layout(location = 1) in float edgeSize;
layout(location = 2) in vec2 position;
layout(location = 3) in float shapeAspectRatio;

void main() {
    color = inColor;
    float edgeRadius = sqrt(2 * edgeSize * edgeSize);
    vec2 topLeftCircleCenter = vec2(edgeRadius);
    vec2 topRightCircleCenter = vec2(shapeAspectRatio - edgeRadius, edgeRadius);
    vec2 bottomLeftCircleCenter = vec2(edgeRadius, 1.0f - edgeRadius);
    vec2 bottomRightCircleCenter = vec2(shapeAspectRatio - edgeRadius, 1.0f - edgeRadius);

    if(position.x < edgeRadius && position.y < edgeRadius)
        color.a *= 1.0f - smoothstep(edgeRadius - 0.05f * edgeRadius, edgeRadius + 0.05f * edgeRadius, distance(position, topLeftCircleCenter));
    if(position.x > shapeAspectRatio - edgeRadius && position.y < edgeRadius)
        color.a *= 1.0f - smoothstep(edgeRadius - 0.05f * edgeRadius, edgeRadius + 0.05f * edgeRadius, distance(position, topRightCircleCenter));
    if(position.x < edgeRadius && position.y > 1.0f - edgeRadius)
        color.a *= 1.0f - smoothstep(edgeRadius - 0.05f * edgeRadius, edgeRadius + 0.05f * edgeRadius, distance(position, bottomLeftCircleCenter));
    if(position.x > shapeAspectRatio - edgeRadius && position.y > 1.0f - edgeRadius)
        color.a *= 1.0f - smoothstep(edgeRadius - 0.05f * edgeRadius, edgeRadius + 0.05f * edgeRadius, distance(position, bottomRightCircleCenter));
}