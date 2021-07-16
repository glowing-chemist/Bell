#include "VertexOutputs.hlsl"
#include "UniformBuffers.hlsl"

[[vk::binding(0)]]
ConstantBuffer<ShadowingLight> light;

[[vk::binding(2)]]
StructuredBuffer<float4x4> skinningBones;

[[vk::binding(3)]]
StructuredBuffer<float4x3> instanceTransforms;

[[vk::push_constant]]
ConstantBuffer<MeshInstanceInfo> model;


ShadowMapVertOutput main(Vertex vertex)
{
	ShadowMapVertOutput output;

	float4x3 meshMatrix = instanceTransforms[model.transformsIndex];
	float4 transformedPositionWS = float4(mul(vertex.position, meshMatrix), 1.0f);

	output.position = mul(light.viewProj, transformedPositionWS);
	output.positionVS = mul(light.view, transformedPositionWS);
	output.uv = vertex.uv;
	output.materialIndex = model.materialIndex;
	output.materialFlags = model.materialFlags;

	return output;
}