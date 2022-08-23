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
#pragma once

#define GLM_FORCE_SWIZZLE
#include <cstddef>
#include <glm/glm.hpp>

#define GLSL_C_TEST

using namespace glm;

struct Sphere {
	vec2 center;
	float radiusSquared;
};

using uint = uint32_t;

constexpr float pi = 3.14159265359;
constexpr float pi3 = pi * pi * pi;
constexpr uint nSamples = 4000;

ivec2 lutSize = ivec2(256, 64);
vec4 betaExtinctionZeroMie = vec4(0.004440f);
vec4 betaExtinctionZeroRayleigh = vec4(0.008612455256387713, 0.013011847505420919, 0.028268803129247174, 1.0f);
#define rayleighScattering betaExtinctionZeroRayleigh
#define mieScattering betaExtinctionZeroMie.x
vec4 absorptionZeroOzone = vec4(0.002366656826034617f, 0.001620425091578396f, 0.00010373049232375621f, 1.0f);
float heightScaleRayleigh = 8;
float heightScaleMie = 1.2;
float layerHeightOzone = 25.0f;
float layer0ConstantFactorOzone = -2.0f / 3.0f;
float layer1ConstantFactorOzone = 8.0f / 3.0f;
float heightRangeOzone = 15.0f;
float maxHeight = 100.0f;
float groundRadius = 6360.0f;

Sphere sunSphere =  {
	.center = vec2(radians(60.0f), radians(10.0f)),
	.radiusSquared = radians(5.0f)
};

vec3 sunLuminance = vec3(1.0f);
