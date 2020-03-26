#include "UniformBuffers.hlsl"
#include "VertexOutputs.hlsl"

[[vk::binding(0)]]
ConstantBuffer<CameraBuffer> camera;

[[vk::binding(2)]]
StructuredBuffer<ObjectMatracies> instanceTransformations;


GBufferVertOutput main(Vertex vertInput, uint instanceID : SV_InstanceID)
{
	const float4x4 transFormation = mul(camera.viewProj, instanceTransformations[instanceID].meshMatrix);
	float4 transformedPosition = mul(transFormation, vertInput.position);

	GBufferVertOutput output;

	output.position = mul(transFormation, vertInput.position);
	output.positionWS = mul(instanceTransformations[instanceID].meshMatrix, vertInput.position);
	output.uv = vertInput.uv;
	output.normal = float4(mul((float3x3)instanceTransformations[instanceID].meshMatrix, float3(vertInput.normal.xyz)), 1.0f);
	output.materialID =  vertInput.materialID;

	// Calculate screen space velocity.
	transformedPosition /= transformedPosition.w;
	float4 previousPosition = mul(mul(camera.previousFrameViewProj, instanceTransformations[instanceID].meshMatrix), vertInput.position);
	previousPosition /= previousPosition.w;
	output.velocity = previousPosition.xy - transformedPosition.xy;

	return output;
}