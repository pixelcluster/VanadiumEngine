#pragma once

#define GLM_FORCE_SWIZZLE
#include <glm/glm.hpp>
#include <cstddef>

using namespace glm;

using uint = uint32_t;

constexpr float pi = 3.14159265359;
constexpr float pi3 = pi * pi * pi;
constexpr uint nSamples = 40;

ivec2 lutSize = ivec2(256, 64);
vec3 betaExtinctionZeroMie = vec3(0.004440f);
float heightScaleRayleigh = 8.0f;
float heightScaleMie = 1.2f;
float iorAir = 1.00029;
float molecularDensity = 1.2041;
float maxHeight = 100.0f;
float groundRadius = 6360.0f;