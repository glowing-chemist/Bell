
#include "UniformBuffers.hlsl"

[[vk::binding(0)]]
ConstantBuffer<CameraBuffer> camera;

struct TerrainVertexInput
{
	float3 position : POSITION0;
};

struct TerrainVertexOutput
{
	float4 position : SV_Position;
};

TerrainVertexOutput main(TerrainVertexInput vertex)
{
	TerrainVertexOutput output;
	output.position = mul(camera.viewProj, float4(vertex.position, 1.0f));

	return output;
}