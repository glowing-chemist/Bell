#include "UniformBuffers.hlsl"
#include "VertexOutputs.hlsl"


[[vk::push_constant]]
ConstantBuffer<ObjectMatracies> model;


[[vk::binding(0)]]
ConstantBuffer<CameraBuffer> camera;


GBufferVertOutput main(Vertex vertInput)
{
	GBufferVertOutput output;

	const float4x4 transFormation = mul(camera.viewProj, model.meshMatrix);
	float4 transformedPosition = mul(transFormation, vertInput.position);

	output.position = transformedPosition;
	output.positionWS = mul(model.meshMatrix, vertInput.position);
	output.uv = vertInput.uv;
	output.normal = float4(mul((float3x3)model.meshMatrix, vertInput.normal.xyz), 1.0f);
	output.materialID =  vertInput.materialID;

	// Calculate screen space velocity.
	transformedPosition /= transformedPosition.w;
	float4 previousPosition = mul(mul(camera.previousFrameViewProj, model.meshMatrix), vertInput.position);
	previousPosition /= previousPosition.w;
	output.velocity = previousPosition.xy - transformedPosition.xy;

	return output;
}
