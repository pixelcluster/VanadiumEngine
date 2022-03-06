//! #version 450 core
//! #extension GL_GOOGLE_include_directive : enable


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

#include "common.glsl"

vec3 betaExtinctionRayleigh(float height) {
	return betaExtinctionZeroRayleigh.rgb * exp(-(height - groundRadius) / heightScaleRayleigh);
}

float betaExtinctionMie(float height) {
	return betaExtinctionZeroMie.r * exp(-(height - groundRadius) / heightScaleMie);
}

vec3 betaExtinctionOzone(float height) {
	float densityFactor = (height - groundRadius) > layerHeightOzone ? -(height - groundRadius) / heightRangeOzone : (height - groundRadius) / heightRangeOzone;
	densityFactor += (height - groundRadius) > layerHeightOzone ? layer1ConstantFactorOzone : layer0ConstantFactorOzone;
	return absorptionZeroOzone.rgb * clamp(densityFactor, 0.0f, 1.0f);
}

vec3 calcTransmittance(vec2 pos, vec2 direction, float deltaT) {
	vec3 result = vec3(0.0f);
	
	float mieExtinction = betaExtinctionMie(length(pos));
	vec3 rayleighExtinction = betaExtinctionRayleigh(length(pos));
	vec3 ozoneExtinction = betaExtinctionOzone(length(pos));
	result += (rayleighExtinction + mieExtinction + ozoneExtinction) * 0.5f;
	for(uint i = 1; i < nSamples - 1; ++i, pos += direction * deltaT) {
		mieExtinction = betaExtinctionMie(length(pos));
		rayleighExtinction = betaExtinctionRayleigh(length(pos));
		ozoneExtinction = betaExtinctionOzone(length(pos));
		result += (rayleighExtinction + mieExtinction + ozoneExtinction);
	}
	mieExtinction = betaExtinctionMie(length(pos));
	rayleighExtinction = betaExtinctionRayleigh(length(pos));
	ozoneExtinction = betaExtinctionOzone(length(pos));
	result += (rayleighExtinction + mieExtinction + ozoneExtinction) * 0.5f;
	result = exp(-result * deltaT);
	return result;
}