#include "AtmosphereParamsCommon.hpp"
#include <TestUtilCommon.hpp>
#include <iostream>

#include "../../../atmosphere/shaders/functions/transmittance-compute.glsl"

int main() {
	float hitLen = nearestDistanceToSphere(groundRadius + 1.3, vec2(0.0f, 1.0f), groundRadius + maxHeight);
	vec3 transmittance = calcTransmittance(vec2(0.0f, groundRadius + 1.3f), vec2(1.0f, 1.0f),
										   nearestDistanceToSphere(1.3f, vec2(1.0f, 1.0f), groundRadius + maxHeight));

	testFloatEqualWithError(0.018033437849686f, transmittance.x, "Transmittance", 0.1f);

	transmittance = calcTransmittance(vec2(0.0f, groundRadius + 1.3f), vec2(1.0f, 0.0f),
										   nearestDistanceToSphere(1.3f, vec2(1.0f, 0.0f), groundRadius + maxHeight));

	testFloatEqualWithError(1.2200237247011f, transmittance.x, "Transmittance horizontal", 0.1f);
}