
float lineariseDepth(const float depth, const float near, const float far)
{
	return  (2.0f * near) / (far + near - depth * (far - near));
}


float calculateLuminosityRGB(const vec3 colour)
{
	return 0.2126f * colour.x + 0.7152f * colour.y + 0.0722f * colour.z;
}


vec3 calculateYCoCg(const vec3 rgb)
{
	const mat3 RGBToYCoCg = mat3(	vec3(0.25f, 0.5f, 0.25f), 
									vec3(0.5f, 0.0f, -0.5f), 
									vec3(-0.25, 0.5f, -0.25f));
	return RGBToYCoCg * rgb;
}


vec3 calculateRGB(const vec3 YCoCg)
{
	const mat3 YCoCgToRGB = mat3(	vec3(1.0f, 1.0f, -1.0f),
									vec3(1.0f, 0.0f, 1.0f),
									vec3(1.0f, -1.0f, -1.0f));

	return YCoCgToRGB * YCoCg;
}
