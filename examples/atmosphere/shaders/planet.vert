#version 450 core

layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 texCoord;

layout(set = 0, binding = 0) uniform SceneData {
	mat4 viewProjection;
};

layout(location = 0) out vec2 outTexCoord;

void main() {
	gl_Position = viewProjection * vec4(pos, 1.0f);
	outTexCoord = texCoord;
	gl_PointSize = 1.0f;
}