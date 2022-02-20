//? #version 450 core

vec3 betaExtinctionRayleigh(float height) {
	return betaExtinctionZeroRayleigh.rgb * 5.0f * exp(-(height - groundRadius) / heightScaleRayleigh);
}

vec3 betaExtinctionMie(float height) {
	return betaExtinctionZeroMie.rgb * vec3(exp(-(height - groundRadius) / heightScaleMie));
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

vec3 calcTransmittance(vec2 startPos, vec2 direction, float endPosT) {
	vec3 result = vec3(0.0f);

	vec2 currentPos = startPos;
	float deltaT = endPosT / nSamples;

	vec3 lastMieExtinction = betaExtinctionMie(length(startPos));
	vec3 lastRayleighExtinction = betaExtinctionRayleigh(length(startPos));
	for(uint i = 0; i < nSamples; ++i, currentPos += direction * deltaT) {
		vec3 mieExtinction = betaExtinctionMie(length(currentPos));
		vec3 rayleighExtinction = betaExtinctionRayleigh(length(currentPos));
		result += ((rayleighExtinction + lastRayleighExtinction + mieExtinction + lastMieExtinction) * 0.5f) * deltaT;
		lastMieExtinction = mieExtinction;
		lastRayleighExtinction = rayleighExtinction;
	}
	result = exp(-result);
	return result;
}