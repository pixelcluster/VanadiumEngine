#version 450 core

layout(location = 0) in vec3 pos;

layout(set = 0, binding = 0) uniform SceneData {
	mat4 viewProjection;
};

void main() {
	gl_Position = viewProjection * vec4(pos, 1.0f);
}