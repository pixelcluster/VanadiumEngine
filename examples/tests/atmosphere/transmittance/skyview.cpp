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

#define HAS_TRANSMITTANCE_LUT
#include "AtmosphereParamsCommon.hpp"
#include "TestUtilCommon.hpp"

#include "../../../atmosphere/shaders/functions/common.glsl"
#include "definitions.glsl"
#include "glm/gtx/compatibility.hpp"

// From https://github.com/sebh/UnrealEngineSkyAtmosphere
//
//  Copyright (c) 2020 Epic Games, Inc.
//	Permission is hereby granted, free of charge, to any person obtaining a copy
//	of this software and associated documentation files (the "Software"), to deal
//	in the Software without restriction, including without limitation the rights
//	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//	copies of the Software, and to permit persons to whom the Software is
//	furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in all
//	copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//	SOFTWARE.

float getAlbedo(float scattering, float extinction) { return scattering / max(0.001f, extinction); }
float3 getAlbedo(float3 scattering, float3 extinction) { return scattering / max(vec3(0.001f), extinction); }

struct MediumSampleRGB {
	float3 scattering;
	float3 absorption;
	float3 extinction;

	float3 scatteringMie;
	float3 absorptionMie;
	float3 extinctionMie;

	float3 scatteringRay;
	float3 absorptionRay;
	float3 extinctionRay;

	float3 scatteringOzo;
	float3 absorptionOzo;
	float3 extinctionOzo;

	float3 albedo;
};

MediumSampleRGB sampleMediumRGB(float3 WorldPos, AtmosphereParameters Atmosphere) {
	const float viewHeight = length(WorldPos) - groundRadius;

	const float densityMie = exp(-(1.0f / heightScaleMie) * viewHeight);
	const float densityRay = exp(-(1.0f / heightScaleRayleigh) * viewHeight);
	const float densityOzo = saturate(viewHeight < Atmosphere.absorption_density.layers[0].width
										  ? Atmosphere.absorption_density.layers[0].linear_term * viewHeight +
												Atmosphere.absorption_density.layers[0].constant_term
										  : Atmosphere.absorption_density.layers[1].linear_term * viewHeight +
												Atmosphere.absorption_density.layers[1].constant_term);

	MediumSampleRGB s;

	s.scatteringMie = densityMie * Atmosphere.mie_scattering;
	s.absorptionMie = densityMie * Atmosphere.mie_extinction;
	s.extinctionMie = densityMie * Atmosphere.mie_scattering;

	s.scatteringRay = densityRay * Atmosphere.rayleigh_scattering;
	s.absorptionRay = vec3(0.0f);
	s.extinctionRay = s.scatteringRay + s.absorptionRay;

	s.scatteringOzo = vec3(0.0f);
	s.absorptionOzo = densityOzo * Atmosphere.absorption_extinction;
	s.extinctionOzo = s.scatteringOzo + s.absorptionOzo;

	s.scattering = s.scatteringMie + s.scatteringRay + s.scatteringOzo;
	s.absorption = s.absorptionMie + s.absorptionRay + s.absorptionOzo;
	s.extinction = s.extinctionMie + s.extinctionRay + s.extinctionOzo;
	s.albedo = getAlbedo(s.scattering, s.extinction);

	return s;
}

#define PLANET_RADIUS_OFFSET 0.01f

float RayleighPhase(float cosTheta) {
	float factor = 3.0f / (16.0f * PI);
	return factor * (1.0f + cosTheta * cosTheta);
}

float CornetteShanksMiePhaseFunction(float g, float cosTheta) {
	float k = 3.0 / (8.0 * PI) * (1.0 - g * g) / (2.0 + g * g);
	return k * (1.0 + cosTheta * cosTheta) / pow(1.0 + g * g - 2.0 * g * -cosTheta, 1.5);
}

#define USE_CornetteShanks

float hgPhase(float g, float cosTheta) {
#ifdef USE_CornetteShanks
	return CornetteShanksMiePhaseFunction(g, cosTheta);
#else
	// Reference implementation (i.e. not schlick approximation).
	// See http://www.pbr-book.org/3ed-2018/Volume_Scattering/Phase_Functions.html
	float numer = 1.0f - g * g;
	float denom = 1.0f + g * g + 2.0f * g * cosTheta;
	return numer / (4.0f * PI * denom * sqrt(denom));
#endif
}

struct SingleScatteringResult {
	float3 L;			  // Scattered light (luminance)
	float3 OpticalDepth;  // Optical depth (1/m)
	float3 Transmittance; // Transmittance in [0,1] (unitless)
	float3 MultiScatAs1;

	float3 NewMultiScatStep0Out;
	float3 NewMultiScatStep1Out;
};

SingleScatteringResult IntegrateScatteredLuminance(float2 pixPos, float3 WorldPos, float3 WorldDir, float3 SunDir,
												   AtmosphereParameters Atmosphere, bool ground, float SampleCountIni,
												   float DepthBufferValue, bool VariableSampleCount, bool MieRayPhase,
												   float tMaxMax = 9000000.0f) {
	SingleScatteringResult result = {};

	// Compute next intersection with atmosphere or ground
	float3 earthO = float3(0.0f, 0.0f, 0.0f);
	float tBottom = nearestDistanceToSphere(length(WorldPos), vec2(WorldDir.x, WorldDir.y), groundRadius);
	float tTop = nearestDistanceToSphere(length(WorldPos), vec2(WorldDir.x, WorldDir.y), groundRadius + maxHeight);
	float tMax = 0.0f;
	if (tBottom < 0.0f) {
		if (tTop < 0.0f) {
			tMax = 0.0f; // No intersection with earth nor atmosphere: stop right away
			return result;
		} else {
			tMax = tTop;
		}
	} else {
		if (tTop > 0.0f) {
			tMax = min(tTop, tBottom);
		}
	}
	tMax = min(tMax, tMaxMax);

	// Sample count
	float SampleCount = SampleCountIni;
	float SampleCountFloor = SampleCountIni;
	float tMaxFloor = tMax;
	float dt = tMax / SampleCount;

	// Phase functions
	const float uniformPhase = 1.0 / (4.0 * PI);
	const float3 wi = SunDir;
	const float3 wo = WorldDir;
	float cosTheta = dot(wi, wo);
	float MiePhaseValue = hgPhase(0.8, -cosTheta); // mnegate cosTheta because due to WorldDir being a "in" direction.
	float RayleighPhaseValue = RayleighPhase(cosTheta);

#ifdef ILLUMINANCE_IS_ONE
	// When building the scattering factor, we assume light illuminance is 1 to compute a transfert function relative to
	// identity illuminance of 1. This make the scattering factor independent of the light. It is now only linked to the
	// atmosphere properties.
	float3 globalL = 1.0f;
#else
	float3 globalL = float3(1.0f);
#endif

	// Ray march the atmosphere to integrate optical depth
	float3 L = float3(0.0f);
	float3 throughput = float3(1.0);
	float3 OpticalDepth = float3(0.0);
	float t = 0.0f;
	float tPrev = 0.0;
	const float SampleSegmentT = 0.3f;
	for (float s = 0.0f; s < SampleCount; s += 1.0f) {
		if (VariableSampleCount) {
			// More expenssive but artefact free
			float t0 = (s) / SampleCountFloor;
			float t1 = (s + 1.0f) / SampleCountFloor;
			// Non linear distribution of sample within the range.
			t0 = t0 * t0;
			t1 = t1 * t1;
			// Make t0 and t1 world space distances.
			t0 = tMaxFloor * t0;
			if (t1 > 1.0) {
				t1 = tMax;
				//	t1 = tMaxFloor;	// this reveal depth slices
			} else {
				t1 = tMaxFloor * t1;
			}
			// t = t0 + (t1 - t0) * (whangHashNoise(pixPos.x, pixPos.y, gFrameId * 1920 * 1080)); // With dithering
			// required to hide some sampling artefact relying on TAA later? This may even allow volumetric shadow?
			t = t0 + (t1 - t0) * SampleSegmentT;
			dt = t1 - t0;
		} else {
			// t = tMax * (s + SampleSegmentT) / SampleCount;
			//  Exact difference, important for accuracy of multiple scattering
			float NewT = tMax * (s + SampleSegmentT) / SampleCount;
			dt = NewT - t;
			t = NewT;
		}
		float3 P = WorldPos + t * WorldDir;

#if DEBUGENABLED
		if (debugEnabled) {
			float3 Pprev = WorldPos + tPrev * WorldDir;
			float3 TxToDebugWorld = float3(0, 0, -Atmosphere.BottomRadius);
			addGpuDebugLine(TxToDebugWorld + Pprev, TxToDebugWorld + P, float3(0.2, 1, 0.2));
			addGpuDebugCross(TxToDebugWorld + P, float3(0.2, 0.2, 1.0), 0.2);
		}
#endif

		MediumSampleRGB medium = sampleMediumRGB(P, Atmosphere);
		const float3 SampleOpticalDepth = medium.extinction * dt;
		const float3 SampleTransmittance = exp(-SampleOpticalDepth);
		OpticalDepth += SampleOpticalDepth;

		float pHeight = length(P);
		const float3 UpVector = P / pHeight;
		float SunZenithCosAngle = dot(SunDir, UpVector);
		vec2 direction = vec2(sqrt(1.0f - SunZenithCosAngle * SunZenithCosAngle), SunZenithCosAngle);
		float3 TransmittanceToSun =
			calcTransmittance(vec2(0.0f, pHeight), direction,
							  nearestDistanceToSphere(pHeight, direction, groundRadius + maxHeight) / nSamples);

		float3 PhaseTimesScattering;
		if (MieRayPhase) {
			PhaseTimesScattering = medium.scatteringMie * MiePhaseValue + medium.scatteringRay * RayleighPhaseValue;
		} else {
			PhaseTimesScattering = medium.scattering * uniformPhase;
		}

		// Earth shadow
		float tEarth = nearestDistanceToSphere(length(P), vec2(SunDir.x, SunDir.y), groundRadius);
		float earthShadow = tEarth >= 0.0f ? 0.0f : 1.0f;

		// Dual scattering for multi scattering

		float3 multiScatteredLuminance = vec3(0.0f);
#if MULTISCATAPPROX_ENABLED
		multiScatteredLuminance =
			GetMultipleScattering(Atmosphere, medium.scattering, medium.extinction, P, SunZenithCosAngle);
#endif

		float shadow = 1.0f;
#if SHADOWMAP_ENABLED
		// First evaluate opaque shadow
		shadow = getShadow(Atmosphere, P);
#endif

		float3 S = globalL * (earthShadow * shadow * TransmittanceToSun * PhaseTimesScattering +
							  multiScatteredLuminance * medium.scattering);

		// When using the power serie to accumulate all sattering order, serie r must be <1 for a serie to converge.
		// Under extreme coefficient, MultiScatAs1 can grow larger and thus result in broken visuals.
		// The way to fix that is to use a proper analytical integration as proposed in slide 28 of
		// http://www.frostbite.com/2015/08/physically-based-unified-volumetric-rendering-in-frostbite/ However, it is
		// possible to disable as it can also work using simple power serie sum unroll up to 5th order. The rest of the
		// orders has a really low contribution.
#define MULTI_SCATTERING_POWER_SERIE 1

#if MULTI_SCATTERING_POWER_SERIE == 0
		// 1 is the integration of luminance over the 4pi of a sphere, and assuming an isotropic phase function
		// of 1.0/(4*PI)
		result.MultiScatAs1 += throughput * medium.scattering * 1 * dt;
#else
		float3 MS = medium.scattering * vec3(1);
		float3 MSint = (MS - MS * SampleTransmittance) / medium.extinction;
		result.MultiScatAs1 += throughput * MSint;
#endif

		// Evaluate input to multi scattering
		{
			float3 newMS;

			newMS = earthShadow * TransmittanceToSun * medium.scattering * uniformPhase * vec3(1);
			result.NewMultiScatStep0Out += throughput * (newMS - newMS * SampleTransmittance) / medium.extinction;
			//	result.NewMultiScatStep0Out += SampleTransmittance * throughput * newMS * dt;

			newMS = medium.scattering * uniformPhase * multiScatteredLuminance;
			result.NewMultiScatStep1Out += throughput * (newMS - newMS * SampleTransmittance) / medium.extinction;
			//	result.NewMultiScatStep1Out += SampleTransmittance * throughput * newMS * dt;
		}

#if 0
		L += throughput * S * dt;
		throughput *= SampleTransmittance;
#else
		// See slide 28 at http://www.frostbite.com/2015/08/physically-based-unified-volumetric-rendering-in-frostbite/
		float3 Sint = (S - S * SampleTransmittance) / medium.extinction; // integrate along the current step segment
		L += throughput * Sint; // accumulate and also take into account the transmittance from previous steps
		throughput *= SampleTransmittance;
#endif

		tPrev = t;
	}

	/*if (ground && tMax == tBottom && tBottom > 0.0)
	{
		// Account for bounced light off the earth
		float3 P = WorldPos + tBottom * WorldDir;
		float pHeight = length(P);

		const float3 UpVector = P / pHeight;
		float SunZenithCosAngle = dot(SunDir, UpVector);
		float3 TransmittanceToSun = TransmittanceLutTexture.SampleLevel(samplerLinearClamp, uv, 0).rgb;

		const float NdotL = saturate(dot(normalize(UpVector), normalize(SunDir)));
		L += globalL * TransmittanceToSun * throughput * NdotL * Atmosphere.GroundAlbedo / PI;
	}*/

	result.L = L;
	result.OpticalDepth = OpticalDepth;
	result.Transmittance = throughput;
	return result;
}

void SetupEarthAtmosphere(AtmosphereParameters& info) {
	// Values shown here are the result of integration over wavelength power spectrum integrated with paricular
	// function. Refer to https://github.com/ebruneton/precomputed_atmospheric_scattering for details.

	// All units in kilometers
	const float EarthBottomRadius = 6360.0f;
	const float EarthTopRadius = 6460.0f; // 100km atmosphere radius, less edge visible and it contain 99.99% of the
										  // atmosphere medium https://en.wikipedia.org/wiki/K%C3%A1rm%C3%A1n_line
	const float EarthRayleighScaleHeight = 8.0f;
	const float EarthMieScaleHeight = 1.2f;

	// Sun - This should not be part of the sky model...
	// info.solar_irradiance = { 1.474000f, 1.850400f, 1.911980f };
	info.solar_irradiance = { 1.0f, 1.0f,
							  1.0f }; // Using a normalise sun illuminance. This is to make sure the LUTs acts as a
									  // transfert factor to apply the runtime computed sun irradiance over.
	info.sun_angular_radius = 0.004675f;

	// Earth
	info.bottom_radius = EarthBottomRadius;
	info.top_radius = EarthTopRadius;
	info.ground_albedo = { 0.0f, 0.0f, 0.0f };

	// Raleigh scattering
	info.rayleigh_density.layers[0] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
	info.rayleigh_density.layers[1] = { 0.0f, 1.0f, -1.0f / EarthRayleighScaleHeight, 0.0f, 0.0f };
	info.rayleigh_scattering = betaExtinctionZeroRayleigh.xyz(); // 1/km

	// Mie scattering
	info.mie_density.layers[0] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
	info.mie_density.layers[1] = { 0.0f, 1.0f, -1.0f / EarthMieScaleHeight, 0.0f, 0.0f };
	info.mie_scattering = { 0.003996f, 0.003996f, 0.003996f }; // 1/km
	info.mie_extinction = { 0.004440f, 0.004440f, 0.004440f }; // 1/km
	info.mie_phase_function_g = 0.8f;

	// Ozone absorption
	info.absorption_density.layers[0] = { 25.0f, 0.0f, 0.0f, 1.0f / 15.0f, -2.0f / 3.0f };
	info.absorption_density.layers[1] = { 0.0f, 0.0f, 0.0f, -1.0f / 15.0f, 8.0f / 3.0f };
	info.absorption_extinction = absorptionZeroOzone.xyz(); // 1/km

	const double max_sun_zenith_angle = PI * 120.0 / 180.0; // (use_half_precision_ ? 102.0 : 120.0) / 180.0 * kPi;
	info.mu_s_min = (float)cos(max_sun_zenith_angle);
}

int main() {
	float phi = radians(180.0f);
	float theta = radians(20.0f);

	vec3 resLuminance = luminance(
		0.3f + groundRadius, sinf(theta), cosf(theta),
		nearestDistanceToSphere(0.3f + groundRadius, vec2(sinf(theta), cosf(theta)), groundRadius + maxHeight), theta,
		phi, sinf(phi), cosf(phi), false);
	AtmosphereParameters parameters;
	SetupEarthAtmosphere(parameters);
	SingleScatteringResult refResult =
		IntegrateScatteredLuminance(vec2(0.0f), vec3(0.0f, 0.3f + groundRadius, 0.0f),
									vec3(sinf(theta) * cosf(phi), cosf(theta), -sinf(theta) * sinf(phi)),
									vec3(sinf(sunSphere.center.y) * cosf(sunSphere.center.x), cosf(sunSphere.center.y),
										 -sinf(sunSphere.center.x) * sinf(sunSphere.center.y)),
									parameters, false, 40, -1.0f, false, true);

	testFloatEqualWithError(refResult.L.x, resLuminance.x, "SkyView L x", 0.5f);
	testFloatEqualWithError(refResult.L.y, resLuminance.y, "SkyView L y", 0.5f);
	testFloatEqualWithError(refResult.L.z, resLuminance.z, "SkyView L z", 0.5f);
}