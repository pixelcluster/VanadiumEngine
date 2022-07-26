/* VanadiumEngine, a Vulkan rendering toolkit
 * Copyright (C) 2022 Friedrich Vock
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#version 450 core

layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 texCoord;

layout(set = 0, binding = 0) uniform SceneData {
	mat4 viewProjection;
	vec2 screenDim;
	vec3 camFrustumTopLeft;
	vec3 camRight;
	vec3 camUp;
};

layout(location = 0) out vec2 outTexCoord;
layout(location = 1) out vec3 outNormal;

void main() {
	gl_Position = viewProjection * vec4(pos, 1.0f);
	outNormal = pos;
	outTexCoord = texCoord;
	gl_PointSize = 1.0f;
}
