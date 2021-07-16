#include "UniformBuffers.hlsl"
#include "VertexOutputs.hlsl"

[[vk::binding(0)]]
ConstantBuffer<CameraBuffer> camera;

[[vk::binding(2)]]
StructuredBuffer<MeshInstanceInfo> instanceTransformations;

[[vk::binding(3)]]
StructuredBuffer<float4x4> skinningBones;

[[vk::binding(4)]]
StructuredBuffer<float4x3> instanceTransforms;

[[vk::binding(5)]]
StructuredBuffer<float4x3> prevInstanceTransforms;

GBufferVertOutput main(Vertex vertInput, uint instanceID : SV_InstanceID)
{
	GBufferVertOutput output;

	float4x3 meshMatrix = instanceTransforms[model.transformsIndex];
	float4x3 prevMeshMatrix = prevInstanceTransforms[model.transformsIndex];
	float4 transformedPositionWS = float4(mul(vertInput.position, meshMatrix), 1.0f);
	float4 transformedPosition = mul(camera.viewProj, transformedPositionWS);

	output.position = transformedPosition;
	output.positionWS = transformedPositionWS;
	output.uv = vertInput.uv;
	output.normal = float4(normalize(mul(float3(vertInput.normal.xyz), (float3x3)instanceTransformations[instanceID].meshMatrix)), 1.0f);
	output.tangent = float4(normalize(mul(float3(vertInput.tangent.xyz), (float3x3)instanceTransformations[instanceID].meshMatrix)), vertInput.tangent.w);
	output.colour = vertInput.colour;
	output.materialIndex =  instanceTransformations[instanceID].materialIndex;
	output.materialFlags = instanceTransformations[instanceID].materialFlags;

	// Calculate screen space velocity.
	// Calculate screen space velocity.
	transformedPosition /= transformedPosition.w;
	float4 previousPositionWS = float4(mul(vertInput.position, prevMeshMatrix), 1.0f);
	float4 previousPosition = mul(camera.previousFrameViewProj, previousPositionWS);
	previousPosition /= previousPosition.w;
	output.velocity = transformedPosition.xy - previousPosition.xy;

	return output;
}