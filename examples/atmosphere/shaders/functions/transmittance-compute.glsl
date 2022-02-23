//? #version 450 core

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

vec3 calcTransmittance(vec2 pos, vec2 direction, float deltaT) {
	vec3 result = vec3(0.0f);
	
	float mieExtinction = betaExtinctionMie(length(pos));
	vec3 rayleighExtinction = betaExtinctionRayleigh(length(pos));
	vec3 ozoneExtinction = betaExtinctionOzone(length(pos));
	result += (rayleighExtinction + mieExtinction + ozoneExtinction) * 0.5f;
	for(uint i = 0; i < nSamples; ++i, pos += direction * deltaT) {
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