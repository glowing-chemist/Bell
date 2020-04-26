#include "UniformBuffers.hlsl"
#include "VertexOutputs.hlsl"

[[vk::binding(0)]]
ConstantBuffer<CameraBuffer> camera;

[[vk::binding(2)]]
StructuredBuffer<ObjectMatracies> instanceTransformations;


GBufferVertOutput main(Vertex vertInput, uint instanceID : SV_InstanceID)
{
	GBufferVertOutput output;

	float4x4 meshMatrix;
	float4x4 prevMeshMatrix;
	recreateMeshMatracies(instanceTransformations[instanceID], meshMatrix, prevMeshMatrix);
	float4 transformedPositionWS = mul(vertInput.position, meshMatrix);
	transformedPositionWS.w = 1.0f;
	float4 transformedPosition = mul(camera.viewProj, transformedPositionWS);

	output.position = transformedPosition;
	output.positionWS = transformedPositionWS;
	output.uv = vertInput.uv;
	output.normal = float4(mul((float3x3)instanceTransformations[instanceID].meshMatrix, float3(vertInput.normal.xyz)), 1.0f);
	output.materialID =  instanceTransformations[instanceID].materialID;

	// Calculate screen space velocity.
	// Calculate screen space velocity.
	transformedPosition /= transformedPosition.w;
	float4 previousPositionWS = mul(vertInput.position, prevMeshMatrix);
	previousPositionWS.w = 1.0f;
	float4 previousPosition = mul(camera.previousFrameViewProj, previousPositionWS);
	previousPosition /= previousPosition.w;
	output.velocity = previousPosition.xy - transformedPosition.xy;

	return output;
}