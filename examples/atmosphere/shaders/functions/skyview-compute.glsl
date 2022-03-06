//! #version 450 core
//! #extension GL_GOOGLE_include_directive : enable

#include "common.glsl"

vec3 luminance(float height, float cosTheta) {
	
}

vec3 computeLuminance(float height, float cosTheta) {
	vec2 direction = vec2(sqrt(1 - cosTheta * cosTheta), cosTheta);

	float atmosphereT = nearestDistanceToSphere(height, direction, groundRadius + maxHeight);
	float groundT = nearestDistanceToSphere(height, direction, groundRadius);

	float endPosT = atmosphereT;
	bool intersectsWithGround = false;
	if(groundT > 0.0f) {
		intersectsWithGround = true;
		endPosT = groundT;
	}



}