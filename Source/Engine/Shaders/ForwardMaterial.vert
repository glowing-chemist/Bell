#include "VertexOutputs.hlsl"
#include "UniformBuffers.hlsl"


[[vk::push_constant]]
ConstantBuffer<ObjectMatracies> model;


[[vk::binding(0)]]
ConstantBuffer<CameraBuffer> camera;


GBufferVertOutput main(Vertex vertex)
{
	GBufferVertOutput output;

	float4 transformedPositionWS = mul(model.meshMatrix, vertex.position.xyz);
	transformedPositionWS.w = 1.0f;
	float4 transformedPosition = mul(camera.viewProj, transformedPositionWS);

	output.position = transformedPosition;
	output.positionWS = transformedPositionWS;
	output.normal = float4(mul((float3x3)model.meshMatrix, vertex.normal.xyz), 1.0f);
	output.materialID = model.materialID;
	output.uv = vertex.uv;

	// Calculate screen space velocity.
	transformedPosition /= transformedPosition.w;
	float4 previousPositionWS = mul(model.prevMeshMatrix, vertex.position.xyz);
	previousPositionWS.w = 1.0f;
	float4 previousPosition = mul(camera.previousFrameViewProj, previousPositionWS);
	previousPosition /= previousPosition.w;
	output.velocity = previousPosition.xy - transformedPosition.xy;

	return output;
}