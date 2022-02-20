#pragma once

#define GLM_FORCE_SWIZZLE
#include <glm/glm.hpp>
#include <cstddef>

using namespace glm;

using uint = uint32_t;

constexpr float pi = 3.14159265359;
constexpr float pi3 = pi * pi * pi;
constexpr uint nSamples = 4000;

ivec2 lutSize = ivec2(256, 64);
vec3 betaExtinctionZeroMie = vec3(0.004440f);
vec3 betaExtinctionZeroRayleigh = vec3(0.00165058840844324f, 0.00279225093065194f, 0.00638228835424591f);
float heightScaleRayleigh = 8.0f;
float heightScaleMie = 1.2f;
float maxHeight = 100.0f;
float groundRadius = 6360.0f;