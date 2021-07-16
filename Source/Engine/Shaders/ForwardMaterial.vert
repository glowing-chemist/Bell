#include "VertexOutputs.hlsl"
#include "UniformBuffers.hlsl"


[[vk::push_constant]]
ConstantBuffer<MeshInstanceInfo> model;


[[vk::binding(0)]]
ConstantBuffer<CameraBuffer> camera;

[[vk::binding(1)]]
StructuredBuffer<float4x4> skinningBones;

[[vk::binding(2)]]
StructuredBuffer<float4x3> instanceTransforms;

[[vk::binding(3)]]
StructuredBuffer<float4x3> prevInstanceTransforms;

GBufferVertOutput main(Vertex vertex)
{
	GBufferVertOutput output;

	float4x3 meshMatrix = instanceTransforms[model.transformsIndex];
	float4x3 prevMeshMatrix = prevInstanceTransforms[model.transformsIndex];
	float4 transformedPositionWS = float4(mul(vertex.position, meshMatrix), 1.0f);
	float4 transformedPosition = mul(camera.viewProj, transformedPositionWS);
	float4 prevTransformedPositionWS = float4(mul(vertex.position, prevMeshMatrix), 1.0f);
	float4 prevTransformedPosition = mul(camera.previousFrameViewProj, prevTransformedPositionWS);

	output.position = transformedPosition;
	output.curPosition = transformedPosition;
	output.prevPosition = prevTransformedPosition;
	output.positionWS = transformedPositionWS;
	output.normal = float4(normalize(mul(vertex.normal.xyz, (float3x3)meshMatrix)), 1.0f);
	output.tangent = float4(normalize(mul(vertex.tangent.xyz, (float3x3)meshMatrix)), vertex.tangent.w);
	output.colour = vertex.colour;
	output.materialIndex = model.materialIndex;
	output.materialFlags = model.materialFlags;
	output.uv = vertex.uv;

	return output;
}