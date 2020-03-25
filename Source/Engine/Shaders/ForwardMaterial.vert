#include "VertexOutputs.hlsl"
#include "UniformBuffers.hlsl"


[[vk::push_constant]]
ConstantBuffer<ObjectMatracies> model;


[[vk::binding(0)]]
ConstantBuffer<CameraBuffer> camera;


GBufferVertOutput main(Vertex vertex)
{
	GBufferVertOutput output;

	const float4x4 transFormation = mul(camera.viewProj, model.meshMatrix);
	float4 transformedPosition = mul(transFormation, vertex.position);

	output.position = transformedPosition;
	output.positionWS = mul(model.meshMatrix, vertex.position);
	output.normal = float4(mul((float3x3)model.meshMatrix, vertex.normal.xyz), 1.0f);
	output.materialID = vertex.materialID;
	output.uv = vertex.uv;

	// Calculate screen space velocity.
	transformedPosition /= transformedPosition.w;
	float4 previousPosition = mul(mul(camera.previousFrameViewProj, model.prevMeshMatrix), vertex.position);
	previousPosition /= previousPosition.w;
	output.velocity = previousPosition.xy - transformedPosition.xy;

	return output;
}