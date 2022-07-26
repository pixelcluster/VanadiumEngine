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
#include "AtmosphereParamsCommon.hpp"
#include <TestUtilCommon.hpp>
#include <iostream>

#include "../../../atmosphere/shaders/functions/common.glsl"

int main() {
	float mieExtinction = betaExtinctionMie(groundRadius + 1.3f);
	testFloatEqualWithError(0.338521f, mieExtinction, "Transmittance at 1.3km", 0.1f);
	mieExtinction = betaExtinctionMie(groundRadius + 10.3f);
	testFloatEqualWithError(0.000187231f, mieExtinction, "Transmittance at 10.3km", 0.1f);
	mieExtinction = betaExtinctionMie(groundRadius + 53.3f);
	testFloatEqualWithError(5.13047e-20f, mieExtinction, "Transmittance at 53.3km", 0.1f);
	mieExtinction = betaExtinctionMie(groundRadius + 72.3f);
	testFloatEqualWithError(6.8207e-27f, mieExtinction, "Transmittance at 72.3km", 0.1f);
}
