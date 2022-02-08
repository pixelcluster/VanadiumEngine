#version 450 core

layout(location = 0) in vec3 pos;

layout(set = 0, binding = 0) uniform SceneData {
	mat4 viewProjection;
};

layout(location = 0) out vec3 outPos;

void main() {
	gl_Position = viewProjection * vec4(pos, 1.0f);
	outPos = pos;
	gl_PointSize = 1.0f;
}