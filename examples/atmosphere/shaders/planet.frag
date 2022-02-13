#version 450 core
//editor config
//! #extension GL_KHR_vulkan_glsl : require

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 inTexCoord;
layout(location = 1) in vec3 sphereNormal;

layout(set = 0, binding = 0) uniform SceneData {
	mat4 viewProjection;
	vec2 screenDim;
	vec3 camFrustumTopLeft;
	vec3 camRight;
	vec3 camUp;
};

layout(set = 1, binding = 0) uniform sampler2D tex;
layout(set = 1, binding = 1) uniform sampler2D seaMask;

const float PI = 3.14159265359;

float fresnel(float cosThetaI, float ior) {
	float curEtaI = 1.0f;
	float curEtaT = ior;

	if(cosThetaI < 0.0f) {
		curEtaI = ior;
		curEtaT = 1.0f;
		cosThetaI = -cosThetaI;
	}

	float sinThetaI = sqrt(max(1.0f - cosThetaI * cosThetaI, 0.0f));
	float sinThetaT = curEtaI * sinThetaI / curEtaT;
	float cosThetaT = sqrt(max(1.0f - sinThetaT * sinThetaT, 0.0f));

	float rParallel =	   (curEtaT * cosThetaI - curEtaI * cosThetaT) / 
						   (curEtaT * cosThetaI + curEtaI * cosThetaT);
	float rPerpendicular = (curEtaI * cosThetaI - curEtaT * cosThetaT) / 
						   (curEtaI * cosThetaI + curEtaT * cosThetaT);

	if(sinThetaT >= 1.0f) {
		return 1.0f;
	}

	return (rParallel * rParallel + rPerpendicular * rPerpendicular) / 2.0f;
}

float trowbridgeReitzD(float roughness2, float cos2Theta, float tan2Theta) {
	if(isinf(tan2Theta)) return 0.0f;
	float cos4Theta = cos2Theta * cos2Theta;

	return 1.0f / (PI * roughness2 * cos4Theta * (1.0f + tan2Theta / roughness2) * (1.0f + tan2Theta / roughness2));
}

float trowbridgeReitzLambda(float roughness2, inout vec3 w, inout vec3 normal) {
	float cos2Theta = dot(w, normal) * dot(w, normal);
	float sin2Theta = 1.0f - cos2Theta;
	float absTanTheta = abs(sqrt(sin2Theta / cos2Theta));
	if(cos2Theta == 0.0f)
		absTanTheta = 1.0f;
	
	float cos2Phi = clamp(w.x / sqrt(sin2Theta), -1.0f, 1.0f);
	if(sin2Theta == 0.0f)
		cos2Phi = 1.0f;
	float sin2Phi = 1.0f - cos2Phi;


	float alpha = sqrt(roughness2);
	return (-1.0f + sqrt(1.0f + (alpha * absTanTheta) * (alpha * absTanTheta))) / 2.0f;
}

float trowbridgeReitzG(float roughness2, inout vec3 wo, inout vec3 wi, inout vec3 normal) {
	 return 1.0f / (1.0f + trowbridgeReitzLambda(roughness2, wo, normal) + trowbridgeReitzLambda(roughness2, wi, normal));
}

float torranceSparrowBRDF(float roughness2, vec3 wi, vec3 wo, vec3 normal, float ior) {
	vec3 wh = normalize(wi + wo);
	float cos2ThetaH = dot(wh, normal) * dot(wh, normal);
	float sin2ThetaH = 1.0f - cos2ThetaH;
	float tan2ThetaH = sin2ThetaH / cos2ThetaH;

	float cosThetaI = max(dot(wi, normal), 0.0f);
	float cosThetaO = abs(dot(wo, normal));

	if(cosThetaI == 0.0f)
		return 0.0f;

	return trowbridgeReitzD(roughness2, cos2ThetaH, tan2ThetaH) * trowbridgeReitzG(roughness2, wo, wi, normal) * fresnel(dot(wi, wh), ior) / (4.0f * cosThetaI * cosThetaO);
}

void main() {
	//https://en.wikipedia.org/wiki/Relative_luminance
	bool isSea = dot(texture(seaMask, inTexCoord).rgb, vec3(0.2126, 0.7152, 0.0722)) > 0.01f;
	vec3 lightDir = normalize(sphereNormal - vec3(-3.0f, 0.0f, 0.0));
	vec2 normalizedScreenCoords = (gl_FragCoord.xy - vec2(0.5f)) / screenDim;
	vec3 viewDir = normalize(camFrustumTopLeft + normalizedScreenCoords.x * camRight + normalizedScreenCoords.y * camUp);

	float roughness2, ior;
	if(isSea) {
		roughness2 = 0.001f;
		ior = 1.333f;
	}
	else {
		roughness2 = 1.5f;
		ior = 1.86f;
	}

	outColor = vec4(texture(tex, inTexCoord).rgb * torranceSparrowBRDF(roughness2, -lightDir, viewDir, sphereNormal, ior) * 500.0f, 1.0f);
}