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

	output.position = transformedPosition;
	output.positionWS = transformedPositionWS;
	output.uv = vertInput.uv;
	output.normal = float4(normalize(mul(vertInput.normal.xyz, (float3x3)meshMatrix)), 1.0f);
	output.colour = vertInput.colour;
	output.materialIndex =  model.materialIndex;
	output.materialFlags = model.materialFlags;

	// Calculate screen space velocity.
	transformedPosition /= transformedPosition.w;
	float4 previousPositionWS = mul(vertInput.position, prevMeshMatrix);
	float4 previousPosition = mul(camera.previousFrameViewProj, previousPositionWS);
	previousPosition /= previousPosition.w;
	output.velocity = previousPosition.xy - transformedPosition.xy;

	return output;
}