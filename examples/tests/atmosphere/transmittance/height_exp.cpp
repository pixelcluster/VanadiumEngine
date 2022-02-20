#include "AtmosphereParamsCommon.hpp"
#include <TestUtilCommon.hpp>
#include <iostream>

#include "../../../atmosphere/shaders/functions/transmittance-compute.glsl"

int main() {
	vec3 mieExtinction = betaExtinctionMie(groundRadius + 1.3f);
	testFloatEqualWithError(0.001502786487474f, mieExtinction.x, "Transmittance at 1.3km", 0.1f);
	mieExtinction = betaExtinctionMie(groundRadius + 10.3f);
	testFloatEqualWithError(8.3116771789124e-7f, mieExtinction.x, "Transmittance at 10.3km", 0.1f);
	mieExtinction = betaExtinctionMie(groundRadius + 53.3f);
	testFloatEqualWithError(2.2775595039085e-22f, mieExtinction.x, "Transmittance at 53.3km", 0.1f);
	mieExtinction = betaExtinctionMie(groundRadius + 72.3f);
	testFloatEqualWithError(3.0278923828988e-29f, mieExtinction.x, "Transmittance at 72.3km", 0.1f);
}