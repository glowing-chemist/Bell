#include "VertexOutputs.hlsl"

struct DeferredGBuffer
{
	float3 normal;
	float2 uv;
	uint materialID;
	float2 velocity;
};

DeferredGBuffer main(GBufferVertOutput vertInput)
{
	DeferredGBuffer output;

	output.normal = (vertInput.normal.xyz + 1.0f) * 0.5f;

	// calculate the partial derivitves of the uvs so that we can do manual LOD selection
	// in the lighting shaders. Will also help us calculate tangent space matracies later.
	float4 uvWithDerivitives;
	uvWithDerivitives.xy = vertInput.uv;

	// Pack the derivitives in to a single float this should still give us plenty of precision.
	uint2 packedXDerivitives = f32tof16(ddx_fine(vertInput.uv));
	uint2 packedYDerivitives = f32tof16(ddy_fine(vertInput.uv));

	uvWithDerivitives.z = asfloat(packedXDerivitives.x | packedXDerivitives.y << 16);
	uvWithDerivitives.w = asfloat(packedYDerivitives.x | packedYDerivitives.y << 16);

	output.uv = uvWithDerivitives;
	output.materialID = vertInput.materialID;
	output.velocity = (vertInput.velocity * 0.5f) + 0.5f;

	return output;
}