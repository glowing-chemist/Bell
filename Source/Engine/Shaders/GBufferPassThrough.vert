#include "UniformBuffers.hlsl"
#include "VertexOutputs.hlsl"


[[vk::push_constant]]
ConstantBuffer<ObjectMatracies> model;


[[vk::binding(0)]]
ConstantBuffer<CameraBuffer> camera;


GBufferVertOutput main(Vertex vertInput)
{
	GBufferVertOutput output;

	float4x4 meshMatrix;
	float4x4 prevMeshMatrix;
	recreateMeshMatracies(model.meshMatrix, model.prevMeshMatrix, meshMatrix, prevMeshMatrix);
	float4 transformedPositionWS = mul(vertInput.position, meshMatrix);
	float4 transformedPosition = mul(camera.viewProj, transformedPositionWS);
	float4 prevTransformedPositionWS = mul(vertInput.position, prevMeshMatrix);
	float4 prevTransformedPosition = mul(camera.previousFrameViewProj, prevTransformedPositionWS);

	output.position = transformedPosition;
	output.curPosition = transformedPosition;
	output.prevPosition = prevTransformedPosition;
	output.positionWS = transformedPositionWS;
	output.uv = vertInput.uv;
	output.normal = float4(normalize(mul(vertInput.normal.xyz, (float3x3)meshMatrix)), 1.0f);
	output.tangent = float4(normalize(mul(vertInput.tangent.xyz, (float3x3)meshMatrix)), vertInput.tangent.w);
	output.colour = vertInput.colour;
	output.materialIndex =  model.materialIndex;
	output.materialFlags = model.materialFlags;

	return output;
}