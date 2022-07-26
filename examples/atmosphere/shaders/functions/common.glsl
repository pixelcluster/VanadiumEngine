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
//? #version 450 core
// For the code from UnrealEngineSkyAtmosphere:
// Copyright (c) 2020 Epic Games, Inc.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

//! void main() {}
//! float groundRadius;
//! float maxHeight;


// https://github.com/sebh/UnrealEngineSkyAtmosphere/blob/master/Resources/SkyAtmosphereCommon.hlsl#L142 ported to GLSL with minor modifications
// - h0 - origin height
// - rd: normalized ray direction
// - sR: sphere radius
// - Returns distance from r0 to first intersecion with sphere,
//   or -1.0 if no intersection.
float nearestDistanceToSphere(float h0, vec2 rd, float sR)
{
	float a = dot(rd, rd);
	vec2 s0_r0 = vec2(0.0f, h0) - vec2(0.0f);
	float b = 2.0 * dot(rd, s0_r0);
	float c = dot(s0_r0, s0_r0) - (sR * sR);
	float delta = b * b - 4.0f * a * c;
	if (delta < 0.0f || a == 0.0f)
	{
		return -1.0f;
	}
	float sol0 = (-b - sqrt(delta)) / (2.0f *a);
	float sol1 = (-b + sqrt(delta)) / (2.0f *a);
	if (sol0 < 0.0f && sol1 < 0.0f)
	{
		return -1.0f;
	}
	if (sol0 < 0.0f)
	{
		return max(0.0f, sol1);
	}
	else if (sol1 < 0.0f)
	{
		return max(0.0f, sol0);
	}
	return max(0.0f, min(sol0, sol1));
}

//from https://github.com/sebh/UnrealEngineSkyAtmosphere ported to GLSL

#ifndef GLSL_C_TEST

void uvToHeightCosTheta(vec2 uv, out float height, out float cosTheta) {
	float x_mu = uv.x;
	float x_r = uv.y;

	float H = sqrt((groundRadius + maxHeight) * (groundRadius + maxHeight) - groundRadius * groundRadius);
	float rho = H * x_r;
	height = sqrt(rho * rho + groundRadius * groundRadius);

	float d_min = (groundRadius + maxHeight) - height;
	float d_max = rho + H;
	float d = d_min + x_mu * (d_max - d_min);
	cosTheta = d == 0.0 ? 1.0f : (H * H - rho * rho - d * d) / (2.0 * height * d);
	cosTheta = clamp(cosTheta, -1.0, 1.0);
}

vec2 heightCosThetaToUv(float height, float cosTheta)
{
	float H = sqrt(max((groundRadius + maxHeight) * (groundRadius + maxHeight) - groundRadius * groundRadius, 0.0f));
	float rho = sqrt(max(height * height - groundRadius * groundRadius, 0.0f));

	float discriminant = height * height * (cosTheta * cosTheta - 1.0) + (groundRadius + maxHeight) * (groundRadius + maxHeight);
	float d = max(0.0, (-height * cosTheta + sqrt(discriminant)));

	float d_min = (groundRadius + maxHeight) - height;
	float d_max = rho + H;
	float x_mu = (d - d_min) / (d_max - d_min);
	float x_r = rho / H;

	return vec2(x_mu, x_r);
}

#endif
//! float groundRadius;
//! float heightScaleRayleigh;
//! vec3 betaExtinctionZeroRayleigh;
//! float heightScaleMie;
//! vec3 betaExtinctionZeroMie;
//! float layerHeightOzone;
//! float heightRangeOzone;
//! float layer0ConstantFactorOzone;
//! float layer1ConstantFactorOzone;
//! vec3 absorptionZeroOzone;
//! uint nSamples;

float betaExtinctionRayleigh(float height) {
	return exp(-(height - groundRadius) / heightScaleRayleigh);
}

float betaExtinctionMie(float height) {
	return exp(-(height - groundRadius) / heightScaleMie);
}

float betaExtinctionOzone(float height) {
	float densityFactor = (height - groundRadius) > layerHeightOzone ? -(height - groundRadius) / heightRangeOzone : (height - groundRadius) / heightRangeOzone;
	densityFactor += (height - groundRadius) > layerHeightOzone ? layer1ConstantFactorOzone : layer0ConstantFactorOzone;
	return clamp(densityFactor, 0.0f, 1.0f);
}

#ifndef GLSL_C_TEST
const uint nSamples = 40;
#endif

vec3 calcTransmittance(vec2 pos, vec2 direction, float deltaT) {
	vec3 result = vec3(0.0f);

	vec3 rayleighExtinctionZero = vec3(betaExtinctionZeroRayleigh.r, betaExtinctionZeroRayleigh.g, betaExtinctionZeroRayleigh.b);
	vec3 ozoneExtinctionZero = vec3(absorptionZeroOzone.r, absorptionZeroOzone.g, absorptionZeroOzone.b);
	
	float mieExtinction = betaExtinctionMie(length(pos)) * betaExtinctionZeroMie.r;
	vec3 rayleighExtinction = betaExtinctionRayleigh(length(pos)) * rayleighExtinctionZero;
	vec3 ozoneExtinction = betaExtinctionOzone(length(pos)) * ozoneExtinctionZero;
	result += (rayleighExtinction + mieExtinction + ozoneExtinction) * 0.5f;

	pos += direction * deltaT;

	mieExtinction = 0.0f;
	rayleighExtinction = vec3(0.0f);
	ozoneExtinction = vec3(0.0f);
	for (uint i = 1; i < nSamples; ++i, pos += direction * deltaT) {
		mieExtinction += betaExtinctionMie(length(pos));
		rayleighExtinction += betaExtinctionRayleigh(length(pos));
		ozoneExtinction += betaExtinctionOzone(length(pos));
	}
	result += rayleighExtinction * rayleighExtinctionZero + mieExtinction * betaExtinctionZeroMie.r +
			  ozoneExtinction * ozoneExtinctionZero;

	mieExtinction = betaExtinctionMie(length(pos)) * betaExtinctionZeroMie.r;
	rayleighExtinction = betaExtinctionRayleigh(length(pos)) * rayleighExtinctionZero;
	ozoneExtinction = betaExtinctionOzone(length(pos)) * ozoneExtinctionZero;
	result += (rayleighExtinction + mieExtinction + ozoneExtinction) * 0.5f;

	result *= deltaT;
	result = exp(-result);

	return result;
}

#ifndef GLSL_C_TEST
const float pi = 3.14159265359;
#endif
const float halfPi = 1.57079633;
const float sqrt2 = 1.41421356;
const float halfSqrt2 = 0.707106781;

float compressedLatitudeFromTexcoord(float v) {
	if (v < 0.5) {
		float sqrtResult = sqrt2 * v - halfSqrt2;
		return -sqrtResult * sqrtResult * pi;
	}
	else if (v == 0.5) return 0.0f;
	else {
		return halfPi * (4 * v * v - 4 * v + 1);
	}
}

float texcoordFromCompressedLatitude(float l) {
	return 0.5 + 0.5 * sign(l) * sqrt(abs(l) / halfPi);
}
