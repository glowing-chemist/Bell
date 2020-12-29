#include "VertexOutputs.hlsl"
#include "UniformBuffers.hlsl"

[[vk::binding(0)]]
ConstantBuffer<CameraBuffer> camera;

TerrainVertexOutput main(BasicVertex vertex)
{
	TerrainVertexOutput output;
	const float4 position = mul(camera.viewProj, vertex.position);
	const float4 prevPosition = mul(camera.previousFrameViewProj, vertex.position);
	output.position = position;
	output.clipPosition = position;
	output.prevClipPosition = prevPosition;
	output.worldPosition = vertex.position;
	output.normal = vertex.normal.xyz;

	return output;
}