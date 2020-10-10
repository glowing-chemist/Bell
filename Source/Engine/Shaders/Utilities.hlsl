
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

uint packColour(const float4 colour)
{
    return uint(colour.x * 255.0f) | (uint(colour.y * 255.0f) << 8) | (uint(colour.z * 255.0f) << 16) | (uint(colour.w * 255.0f) << 24);
}

float4 unpackColour(const uint colour)
{
    return float4((colour & 0xFF) / 255.0f, ((colour & 0xFF00) >> 8) / 255.0f, ((colour & 0xFF0000) >> 16) / 255.0f, ((colour & 0xFF000000) >> 24) / 255.0f);
}

Vertex readVertexFromBuffer(ByteAddressBuffer vertexBuffer, uint offset)
{
	Vertex vert;

	vert.position = asfloat(vertexBuffer.Load4(offset));
	vert.uv = asfloat(vertexBuffer.Load2(offset + 16));
	uint vertToProcessPackedNormal = vertexBuffer.Load(offset + 24);
	vert.normal = unpackNormals(vertToProcessPackedNormal);
	vert.colour = unpackColour(vertexBuffer.Load(offset + 28));

	return vert;
}

void writeVertexToBuffer(RWByteAddressBuffer vertexBuffer, uint offset, Vertex vert)
{
	vertexBuffer.Store4(offset, asuint(vert.position));
	vertexBuffer.Store2(offset + 16, asuint(vert.uv));
	vertexBuffer.Store(offset + 24, packNormals(vert.normal));
	vertexBuffer.Store(offset + 28, packColour(vert.colour));
}