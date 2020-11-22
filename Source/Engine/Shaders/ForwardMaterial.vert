#include "VertexOutputs.hlsl"
#include "UniformBuffers.hlsl"


[[vk::push_constant]]
ConstantBuffer<ObjectMatracies> model;


[[vk::binding(0)]]
ConstantBuffer<CameraBuffer> camera;


GBufferVertOutput main(Vertex vertex)
{
	GBufferVertOutput output;

	float4x4 meshMatrix;
	float4x4 prevMeshMatrix;
	recreateMeshMatracies(model.meshMatrix, model.prevMeshMatrix, meshMatrix, prevMeshMatrix);
	float4 transformedPositionWS = mul(vertex.position, meshMatrix);
	float4 transformedPosition = mul(camera.viewProj, transformedPositionWS);
	float4 prevTransformedPositionWS = mul(vertex.position, prevMeshMatrix);
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