#include "AtmosphereParamsCommon.hpp"
#include <TestUtilCommon.hpp>
#include <iostream>

#include "../../../atmosphere/shaders/functions/transmittance-compute.glsl"

int main() {
	float mieExtinction = betaExtinctionMie(groundRadius + 1.3f);
	testFloatEqualWithError(0.00285576f, mieExtinction, "Transmittance at 1.3km", 0.1f);
	mieExtinction = betaExtinctionMie(groundRadius + 10.3f);
	testFloatEqualWithError(1.57948e-06f, mieExtinction, "Transmittance at 10.3km", 0.1f);
	mieExtinction = betaExtinctionMie(groundRadius + 53.3f);
	testFloatEqualWithError(4.32807e-22f, mieExtinction, "Transmittance at 53.3km", 0.1f);
	mieExtinction = betaExtinctionMie(groundRadius + 72.3f);
	testFloatEqualWithError(5.75394e-29f, mieExtinction, "Transmittance at 72.3km", 0.1f);
}