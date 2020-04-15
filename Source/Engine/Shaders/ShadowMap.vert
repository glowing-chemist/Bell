#include "VertexOutputs.hlsl"
#include "UniformBuffers.hlsl"

[[vk::binding(0)]]
ConstantBuffer<ShadowingLight> light;

[[vk::push_constant]]
ConstantBuffer<ObjectMatracies> model;


ShadowMapVertOutput main(Vertex vertex)
{
	ShadowMapVertOutput output;

	float4 transformedPositionWS = mul(model.meshMatrix, vertex.position.xyz);
	transformedPositionWS.w = 1.0f;

	output.position = mul(light.viewProj, transformedPositionWS);
	output.positionVS = mul(light.view, transformedPositionWS);
	output.uv = vertex.uv;
	output.materialID = model.materialID;

	return output;
}