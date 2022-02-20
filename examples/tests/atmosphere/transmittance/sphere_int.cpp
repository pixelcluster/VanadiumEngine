#include "AtmosphereParamsCommon.hpp"
#include <iostream>
#include <TestUtilCommon.hpp>

#include "../../../atmosphere/shaders/functions/transmittance-compute.glsl"

int main() { 
	float distTop = nearestDistanceToSphere(groundRadius + 30.0f, vec2(0.0f, 1.0f), groundRadius + maxHeight);
	float distGround = nearestDistanceToSphere(groundRadius + 30.0f, vec2(0.0f, -1.0f), groundRadius);
	testFloatEqualWithError(70.0f, distTop, "Distance to sky top");
	testFloatEqualWithError(30.0f, distGround, "Distance to ground");

	float distHorz = nearestDistanceToSphere(groundRadius, vec2(1.0f, 0.0f), groundRadius + maxHeight);
	testFloatEqualWithError(1200.0f, distHorz, "Horizontal distance to top", 10.0f);
}