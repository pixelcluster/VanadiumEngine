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
#include <iostream>
#include <TestUtilCommon.hpp>

#include "../../../atmosphere/shaders/functions/common.glsl"

int main() { 
	float distTop = nearestDistanceToSphere(groundRadius + 30.0f, vec2(0.0f, 1.0f), groundRadius + maxHeight);
	float distGround = nearestDistanceToSphere(groundRadius + 30.0f, vec2(0.0f, -1.0f), groundRadius);
	testFloatEqualWithError(70.0f, distTop, "Distance to sky top");
	testFloatEqualWithError(30.0f, distGround, "Distance to ground");

	float distHorz = nearestDistanceToSphere(groundRadius, vec2(1.0f, 0.0f), groundRadius + maxHeight);
	testFloatEqualWithError(1200.0f, distHorz, "Horizontal distance to top", 6.0f);
}
