//? #version 450 core
// https://github.com/sebh/UnrealEngineSkyAtmosphere/blob/master/Resources/SkyAtmosphereCommon.hlsl#L142 ported to GLSL with minor modifications
// - h0 - origin height
// - rd: normalized ray direction
// - sR: sphere radius
// - Returns distance from r0 to first intersecion with sphere,
//   or -1.0 if no intersection.
//! void main() {}
//! float groundRadius;
//! float maxHeight;

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

//from https://github.com/sebh/UnrealEngineSkyAtmosphere ported to GLSL

#ifndef GLSL_C_TEST

void uvToHeightCosTheta(vec2 uv, out float height, out float cosTheta) {
	float x_mu = uv.x;
	float x_r = uv.y;

	float H = sqrt((groundRadius + maxHeight) * (groundRadius + maxHeight) - groundRadius * groundRadius);
	float rho = H * x_r;
	height = sqrt(rho * rho + groundRadius * groundRadius);

	float d_min = (groundRadius + maxHeight) - height;
	float d_max = rho + H;
	float d = d_min + x_mu * (d_max - d_min);
	cosTheta = d == 0.0 ? 1.0f : (H * H - rho * rho - d * d) / (2.0 * height * d);
	cosTheta = clamp(cosTheta, -1.0, 1.0);
}

vec2 heightCosThetaToUv(float height, float cosTheta)
{
	float H = sqrt(max((groundRadius + maxHeight) * (groundRadius + maxHeight) - groundRadius * groundRadius, 0.0f));
	float rho = sqrt(max(height * height - groundRadius * groundRadius, 0.0f));

	float discriminant = height * height * (cosTheta * cosTheta - 1.0) + (groundRadius + maxHeight) * (groundRadius + maxHeight);
	float d = max(0.0, (-height * cosTheta + sqrt(discriminant)));

	float d_min = (groundRadius + maxHeight) - height;
	float d_max = rho + H;
	float x_mu = (d - d_min) / (d_max - d_min);
	float x_r = rho / H;

	return vec2(x_mu, x_r);
}

#endif