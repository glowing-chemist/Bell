#include "VertexOutputs.hlsl"
#include "UniformBuffers.hlsl"

[[vk::binding(0)]]
ConstantBuffer<CameraBuffer> camera;

TerrainVertexOutput main(BasicVertex vertex)
{
	TerrainVertexOutput output;
	output.position = mul(camera.viewProj, vertex.position);
	output.worldPosition = vertex.position;
	output.normal = vertex.normal.xyz;

	return output;
}