#pragma once

#define GLM_FORCE_SWIZZLE
#include <cstddef>
#include <glm/glm.hpp>

#define GLSL_C_TEST

using namespace glm;

using uint = uint32_t;

constexpr float pi = 3.14159265359;
constexpr float pi3 = pi * pi * pi;
constexpr uint nSamples = 4000;

ivec2 lutSize = ivec2(256, 64);
vec4 betaExtinctionZeroMie = vec4(0.004440f + 0.003996f);
vec4 betaExtinctionZeroRayleigh = vec4(0.005802f, 0.013558f, 0.331f, 1.0f);
vec4 absorptionZeroOzone = vec4(0.00065f, 0.001881f, 0.000085f, 1.0f);
float heightScaleRayleigh = 8;
float heightScaleMie = 1.2;
float layerHeightOzone = 25.0f;
float layer0ConstantFactorOzone = -2.0f / 3.0f;
float layer1ConstantFactorOzone = 8.0f / 3.0f;
float heightRangeOzone = 15.0f;
float maxHeight = 100.0f;
float groundRadius = 6360.0f;