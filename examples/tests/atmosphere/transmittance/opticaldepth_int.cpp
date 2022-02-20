#include "AtmosphereParamsCommon.hpp"
#define TEST_TRACE_PASSED
#include <TestUtilCommon.hpp>
#include <iostream>

#include "../../../atmosphere/shaders/functions/transmittance-compute.glsl"

int main() {
	float hitLen = nearestDistanceToSphere(groundRadius + 1.3, vec2(0.0f, 1.0f), groundRadius + maxHeight);

	vec3 transmittance = calcTransmittance(vec2(0.0f, groundRadius + 1.3f), vec2(1.0f, 0.0f), 1.0f);
	testFloatEqualWithError(0.99544634472114f, transmittance.x, "Transmittance simplified", 40.0f);

	transmittance = calcTransmittance(vec2(0.0f, groundRadius + 1.3f), vec2(0.0f, 1.0f),
										   nearestDistanceToSphere(1.3f, vec2(0.0f, 1.0f), groundRadius + maxHeight));

	testFloatEqualWithError(0.98705698246647f, transmittance.x, "Transmittance", 40.0f);

	transmittance = calcTransmittance(vec2(0.0f, groundRadius + 1.3f), vec2(1.0f, 0.0f),
										   nearestDistanceToSphere(1.3f, vec2(1.0f, 0.0f), groundRadius + maxHeight));

	testFloatEqualWithError(0.57038695071087f, transmittance.x, "Transmittance horizontal", 40.0f);
}