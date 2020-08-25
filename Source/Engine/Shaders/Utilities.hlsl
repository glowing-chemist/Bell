
float lineariseDepth(const float depth, const float near, const float far)
{
	return  (2.0f * near) / (far + near + depth * (far - near));
}


float lineariseReverseDepth(const float depth, const float near, const float far)
{
	return near / (depth * (far - near));
}

float unlineariseReverseDepth(const float depth, const float near, const float far)
{
	return near / (depth * (far - near));
}


float calculateLuminosityRGB(const float3 colour)
{
	return 0.2126f * colour.x + 0.7152f * colour.y + 0.0722f * colour.z;
}


float3 calculateYCoCg(const float3 rgb)
{
	const float3x3 RGBToYCoCg = float3x3(	float3(0.25f, 0.5f, 0.25f), 
									float3(0.5f, 0.0f, -0.5f), 
									float3(-0.25, 0.5f, -0.25f));
	return mul(rgb, RGBToYCoCg);
}


float3 calculateRGB(const float3 YCoCg)
{
	const float3x3 YCoCgToRGB = float3x3(	float3(1.0f, 1.0f, -1.0f),
									float3(1.0f, 0.0f, 1.0f),
									float3(1.0f, -1.0f, -1.0f));

	return mul(YCoCg, YCoCgToRGB);
}

uint packNormals(const float4 normal)
{
    int4 Packed = int4(round(clamp(normal, -1.0, 1.0) * 127.0)) & 0xff;
    return uint(Packed.x | (Packed.y << 8) | (Packed.z << 16) | (Packed.w << 24));
}

float4 unpackNormals(const uint packedNormal)
{
    int SignedValue = int(packedNormal);
    int4 Packed = int4(SignedValue << 24, SignedValue << 16, SignedValue << 8, SignedValue) >> 24;
    return clamp(float4(Packed) / 127.0, -1.0, 1.0);
}
