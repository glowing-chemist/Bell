#include "VertexOutputs.hlsl"
#include "UniformBuffers.hlsl"

[[vk::binding(0)]]
ConstantBuffer<ShadowingLight> light;

[[vk::push_constant]]
ConstantBuffer<ObjectMatracies> model;


ShadowMapVertOutput main(Vertex vertex)
{
	ShadowMapVertOutput output;

	float4x4 meshMatrix;
	float4x4 prevMeshMatrix;
	recreateMeshMatracies(model.meshMatrix, model.prevMeshMatrix, meshMatrix, prevMeshMatrix);
	float4 transformedPositionWS = mul(vertex.position, meshMatrix);

	output.position = mul(light.viewProj, transformedPositionWS);
	output.positionVS = mul(light.view, transformedPositionWS);
	output.uv = vertex.uv;
	output.materialIndex = model.materialIndex;
	output.materialFlags = model.materialFlags;

	return output;
}