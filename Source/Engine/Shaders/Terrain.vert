#include "VertexOutputs.hlsl"
#include "UniformBuffers.hlsl"

[[vk::binding(0)]]
ConstantBuffer<CameraBuffer> camera;


struct TerrainVertexOutput
{
	float4 position : SV_Position;
	float3 normal : NORMAL;
};

TerrainVertexOutput main(BasicVertex vertex)
{
	TerrainVertexOutput output;
	output.position = mul(camera.viewProj, vertex.position);
	output.normal = vertex.normal.xyz;

	return output;
}